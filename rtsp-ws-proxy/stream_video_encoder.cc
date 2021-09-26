#include "stream_video_encoder.h"

#include "common/media/stream_def.h"
#include "common/util/log.h"
#include "common/util/logext.h"
#include "common/util/throw_error.h"

StreamVideoEncoder::StreamVideoEncoder(const StreamVideoEncodeOptions &options)
  : options_(options), codec_ctx_(nullptr), packet_(nullptr) {
  Init();
}

StreamVideoEncoder::~StreamVideoEncoder() {
  Free();
}

AVCodecContext *StreamVideoEncoder::GetCodecContext() {
  return codec_ctx_;
}

int StreamVideoEncoder::Send(AVFrame *frame) {
  LOG_IF(FATAL, codec_ctx_ == nullptr);
  return avcodec_send_frame(codec_ctx_, frame);
}

int StreamVideoEncoder::Recv(AVPacket *packet) {
  LOG_IF(FATAL, codec_ctx_ == nullptr || packet == nullptr);
  return avcodec_receive_packet(codec_ctx_, packet);
}

void StreamVideoEncoder::Encode(AVFrame *frame,
    std::function<void(AVPacket *)> on_recv) {
  int ret = Send(frame);
  if (ret < 0) throw StreamError(ret);

  if (packet_ == nullptr) {
    packet_ = av_packet_alloc();
  }
  while (ret >= 0) {
    ret = Recv(packet_);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      return;
    } else if (ret < 0) {
      throw StreamError(ret);
    }
    on_recv(packet_);
    av_packet_unref(packet_);
  }
}

int StreamVideoEncoder::Flush() {
  return Send(NULL);
}

void StreamVideoEncoder::Init() {
  if (codec_ctx_ != nullptr) return;

  AVCodec *codec = nullptr;
  if (!options_.codec_name.empty()) {
    codec = avcodec_find_encoder_by_name(options_.codec_name.c_str());
    VLOG_IF(1, codec != nullptr) << "Encoder found: " << options_.codec_name;
  }
  if (codec == nullptr) {
    auto codec_id = AV_CODEC_ID_H264;
    codec = avcodec_find_encoder(codec_id);
    if (codec == nullptr) {
      throw_error<StreamError>() << "Encoder not found, id=" << codec_id;
    }
  }

  codec_ctx_ = avcodec_alloc_context3(codec);
  if (codec_ctx_ == nullptr)
    throw StreamError("Codec alloc context fail");

  if (options_.codec_bit_rate > -1)
    codec_ctx_->bit_rate = options_.codec_bit_rate;
  if (options_.codec_width > -1)
    codec_ctx_->width = options_.codec_width;
  if (options_.codec_height > -1)
    codec_ctx_->height = options_.codec_height;
  if (options_.codec_framerate > -1) {
    codec_ctx_->time_base = AVRational{1, options_.codec_framerate};
    codec_ctx_->framerate = AVRational{options_.codec_framerate, 1};
  }
  if (options_.codec_pix_fmt != AV_PIX_FMT_NONE)
    codec_ctx_->pix_fmt = options_.codec_pix_fmt;
  if (options_.codec_gop_size > -1)
    codec_ctx_->gop_size = options_.codec_gop_size;
  if (options_.codec_max_b_frames > -1)
    codec_ctx_->max_b_frames = options_.codec_max_b_frames;
  if (options_.codec_qmin > -1)
    codec_ctx_->qmin = options_.codec_qmin;
  if (options_.codec_qmax > -1)
    codec_ctx_->qmax = options_.codec_qmax;
  if (options_.codec_thread_count > -1)
    codec_ctx_->thread_count = options_.codec_thread_count;

  AVDictionary *options = nullptr;
  for (auto &&e : options_.open_options) {
    av_dict_set(&options, e.first.c_str(), e.second.c_str(), 0);
  }
  int ret = avcodec_open2(codec_ctx_, codec, &options);
  if (ret != 0) throw StreamError(ret);
}

void StreamVideoEncoder::Free() {
  if (packet_) {
    av_packet_free(&packet_);
    packet_ = nullptr;
  }
  if (codec_ctx_) {
    avcodec_close(codec_ctx_);
    avcodec_free_context(&codec_ctx_);
    codec_ctx_ = nullptr;
  }
}
