#include "stream_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#ifdef __cplusplus
}
#endif

#include <functional>

#include "common/util/log.h"
#include "common/util/logext.h"
#include "common/util/throw_error.h"

StreamHandler::StreamHandler(
    const std::string &id,
    const StreamOptions &options,
    const std::vector<StreamFilterOptions> &filters_options,
    int get_frequency,
    packet_callback_t cb)
  : id_(id), options_(options), filters_options_(filters_options),
    get_frequency_(get_frequency), packet_cb_(cb),
    stream_(nullptr), video_filters_inited_(false), packet_recv_(nullptr) {
  std::stringstream ss;
  ss << "Stream[" << id_ << "]";
  log_id_ = ss.str();
}

StreamHandler::~StreamHandler() {
}

void StreamHandler::Start() {
  auto get_types = {AVMEDIA_TYPE_VIDEO};
  stream_ = std::make_shared<StreamThread>(get_types);
  stream_->SetEventCallback(
      std::bind(&StreamHandler::OnEvent, this, std::placeholders::_1));
  stream_->SetRunningCallback(
      std::bind(&StreamHandler::OnRunning, this,
      std::placeholders::_1, std::placeholders::_2));
  stream_->Start(options_, get_frequency_);
}

void StreamHandler::Stop() {
  stream_->Stop();
  if (packet_recv_) {
    av_packet_free(&packet_recv_);
    packet_recv_ = nullptr;
  }
}

void StreamHandler::OnEvent(const std::shared_ptr<StreamEvent> &e) {
  if (e->id == STREAM_EVENT_OPEN) {
    LOG(INFO) << log_id_ << " open ...";
  } else if (e->id == STREAM_EVENT_OPENED) {
    LOG(INFO) << log_id_ << " open success";
  // } else if (e->id == STREAM_EVENT_CLOSE) {
  //   LOG(INFO) << log_id_ << " close ...";
  } else if (e->id == STREAM_EVENT_CLOSED) {
    LOG(INFO) << log_id_ << " close success";
  } else if (e->id == STREAM_EVENT_LOOP) {
    LOG(WARNING) << log_id_ << " loop ...";
  } else if (e->id == STREAM_EVENT_ERROR) {
    auto event = std::dynamic_pointer_cast<StreamErrorEvent>(e);
    LOG(ERROR) << log_id_ << " " << event->error.what();
  }
}

void StreamHandler::OnRunning(const std::shared_ptr<StreamThread> &thread,
                              const std::shared_ptr<Stream> &s) {
  (void)thread;
  auto t = logext::TimeRecord::Create(log_id_ + " run");

  t->Beg("get_pkt");
  auto packet = s->GetPacket(false);
  if (packet == nullptr) return;
  t->End();

  auto type = AVMEDIA_TYPE_VIDEO;
  auto sub = s->GetStreamSub(type);
  if (sub->stream->index != packet->stream_index) {
    return;
  }

  InitVideoFilters(sub);

  if (video_filters_.empty()) {
    if (packet_cb_) {
      packet_cb_(s, type, packet);
    }
    av_packet_unref(packet);
  } else {
    t->Beg("do_filter");
    auto do_callback = [this, &s, &type](AVPacket *pkt) {
      if (packet_cb_) packet_cb_(s, type, pkt);
    };
    DoFilter(video_filters_, video_filters_.begin(), packet, do_callback);
    t->End();
  }

  VLOG(2) << t->Log();
}

void StreamHandler::DoFilter(
    const std::vector<std::shared_ptr<StreamFilter>> &filters,
    const std::vector<std::shared_ptr<StreamFilter>>::iterator &filter,
    AVPacket *pkt,
    std::function<void(AVPacket *pkt)> on_recv) {
  if (filter == filters.end()) {
    on_recv(pkt);
    return;
  }

  int status;

  // send
  do {
    status = (*filter)->SendPacket(pkt);
    av_packet_unref(pkt);
    if (status == STREAM_FILTER_STATUS_BREAK) return;
  } while (status == STREAM_FILTER_STATUS_AGAIN);

  // recv
  if (packet_recv_ == nullptr) {
    packet_recv_ = av_packet_alloc();
  }
  do {
    status = (*filter)->RecvPacket(packet_recv_);
    if (status == STREAM_FILTER_STATUS_BREAK) {
      av_packet_unref(packet_recv_);
      return;
    }
    DoFilter(filters, filter+1, packet_recv_, on_recv);
    av_packet_unref(packet_recv_);
  } while (status == STREAM_FILTER_STATUS_AGAIN);

  // STREAM_FILTER_STATUS_OK
}

void StreamHandler::InitVideoFilters(
    const std::shared_ptr<Stream::stream_sub_t> &video) {
  if (video_filters_inited_) return;
  for (auto opts : filters_options_) {
    switch (opts.type) {
    case STREAM_FILTER_VIDEO_BSF:
      video_filters_.push_back(std::make_shared<StreamFilterVideoBSF>(
          video, opts));
      break;
    case STREAM_FILTER_VIDEO_ENC:
      video_filters_.push_back(std::make_shared<StreamFilterVideoEnc>(
          video, opts));
      break;
    default: break;
    }
  }
  video_filters_inited_ = true;
}
