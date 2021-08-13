#include "stream_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#ifdef __cplusplus
}
#endif

#include <glog/logging.h>

#include <functional>

#include "common/util/throw_error.h"

StreamHandler::StreamHandler(const std::string &id,
                             const StreamOptions &options,
                             int get_frequency,
                             packet_callback_t cb)
  : id_(id), options_(options), get_frequency_(get_frequency), packet_cb_(cb),
    stream_(nullptr), bsf_ctx_(nullptr), bsf_packet_(nullptr) {
}

StreamHandler::~StreamHandler() {
}

void StreamHandler::Start() {
  auto get_types = {AVMEDIA_TYPE_VIDEO};
  stream_ = std::make_shared<StreamThread>(get_types);
  stream_->SetEventCallback(
      std::bind(&StreamHandler::OnEvent, this, std::placeholders::_1));
  stream_->Start(options_, get_frequency_);
}

void StreamHandler::Stop() {
  stream_->Stop();
  if (bsf_ctx_) {
    av_bsf_free(&bsf_ctx_);
    bsf_ctx_ = nullptr;
  }
  if (bsf_packet_) {
    av_packet_free(&bsf_packet_);
    bsf_packet_ = nullptr;
  }
}

void StreamHandler::OnEvent(std::shared_ptr<StreamEvent> e) {
  if (e->id == STREAM_EVENT_GET_PACKET) {
    OnEventPacket(std::dynamic_pointer_cast<StreamPacketEvent>(e));
  } else if (e->id == STREAM_EVENT_ERROR) {
    auto event = std::dynamic_pointer_cast<StreamErrorEvent>(e);
    LOG(ERROR) << "Stream[" << id_ << "] " << event->error.what();
  } else if (e->id == STREAM_EVENT_OPEN) {
    LOG(INFO) << "Stream[" << id_ << "] open ...";
  } else if (e->id == STREAM_EVENT_OPENED) {
    LOG(INFO) << "Stream[" << id_ << "] open success";
  // } else if (e->id == STREAM_EVENT_CLOSE) {
  //   LOG(INFO) << "Stream[" << id_ << "] close ...";
  } else if (e->id == STREAM_EVENT_CLOSED) {
    LOG(INFO) << "Stream[" << id_ << "] close success";
  }
}

void StreamHandler::OnEventPacket(std::shared_ptr<StreamPacketEvent> e) {
  auto packet = e->packet;
  if (packet == nullptr) return;

  auto stream = e->stream;
  auto stream_video = stream->GetStreamSub(AVMEDIA_TYPE_VIDEO);
  if (stream_video->GetIndex() != packet->stream_index) {
    return;
  }

  if (bsf_ctx_ == nullptr) {
    auto stream_av = stream_video->stream();
    auto codec_id = stream_av->codecpar->codec_id;

    // ./configure --list-bsfs , ffmpeg -hide_banner -bsfs
    std::string bsf_name;
    switch (codec_id) {
    case AV_CODEC_ID_H264: bsf_name = "h264_mp4toannexb"; break;
    case AV_CODEC_ID_HEVC: bsf_name = "hevc_mp4toannexb"; break;
    case AV_CODEC_ID_RAWVIDEO: bsf_name = "null"; break;
    default:
      throw_error<StreamError>() << "BSF name not accept, id=" << codec_id;
    }

    auto bsf = av_bsf_get_by_name(bsf_name.c_str());
    if (bsf == nullptr) {
      throw_error<StreamError>() << "BSF not found, name=" << bsf_name;
    }

    int ret = av_bsf_alloc(bsf, &bsf_ctx_);
    if (ret < 0) throw StreamError(ret);

    ret = avcodec_parameters_copy(bsf_ctx_->par_in, stream_av->codecpar);
    if (ret < 0) throw StreamError(ret);

    ret = av_bsf_init(bsf_ctx_);
    if (ret != 0) throw StreamError(ret);
  }

  if (bsf_packet_ == nullptr) {
    bsf_packet_ = av_packet_alloc();
  }

  int ret = av_bsf_send_packet(bsf_ctx_, packet);
  if (ret != 0) {
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      // need handle or notify special error codes here
      return;
    } else {
      throw StreamError(ret);
    }
  }

  ret = av_bsf_receive_packet(bsf_ctx_, bsf_packet_);
  if (ret != 0) {
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      av_packet_unref(bsf_packet_);
      return;
    } else {
      throw StreamError(ret);
    }
  }

  if (packet_cb_) {
    packet_cb_(stream_video, bsf_packet_);
  }

  av_packet_unref(bsf_packet_);
}
