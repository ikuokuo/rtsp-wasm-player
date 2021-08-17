#include "stream_video.h"

#include <cassert>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

#ifdef __cplusplus
}
#endif

#include "common/util/throw_error.h"

StreamVideo::StreamVideo(const StreamOptions &options, AVStream *stream)
  : StreamSub(stream), options_(options), codec_ctx_(nullptr), frame_(nullptr),
    sws_ctx_(nullptr), sws_frame_(nullptr) {
}

StreamVideo::~StreamVideo() {
  Free();
}

int StreamVideo::GetIndex() {
  return stream_->index;
}

AVFrame *StreamVideo::GetFrame(AVPacket *packet) {
  assert(packet != nullptr);

  // decode

  if (codec_ctx_ == nullptr) {
    AVCodec *codec_ = avcodec_find_decoder(stream_->codecpar->codec_id);
    if (codec_ == nullptr) {
      throw_error<StreamError>() << "Decoder not found, id="
        << stream_->codecpar->codec_id;
    }

    codec_ctx_ = avcodec_alloc_context3(codec_);
    if (codec_ctx_ == nullptr)
      throw StreamError("Codec alloc context fail");

    int ret = avcodec_parameters_to_context(codec_ctx_, stream_->codecpar);
    if (ret < 0) throw StreamError(ret);

    ret = avcodec_open2(codec_ctx_, codec_, nullptr);
    if (ret != 0) throw StreamError(ret);

    frame_ = av_frame_alloc();
  }

  int ret = avcodec_send_packet(codec_ctx_, packet);
  if (ret != 0) {
    if (ret == AVERROR(EAGAIN)) {
      return nullptr;
    } else {
      throw StreamError(ret);
    }
  }

  ret = avcodec_receive_frame(codec_ctx_, frame_);
  if (ret != 0) {
    if (ret == AVERROR(EAGAIN)) {
      return nullptr;
    } else {
      throw StreamError(ret);
    }
  }

  AVFrame *result = frame_;

  // scale

  if (options_.sws_enable) {
    if (sws_ctx_ == nullptr) {
      auto pix_fmt = options_.sws_dst_pix_fmt;
      int width = options_.sws_dst_width;
      int height = options_.sws_dst_height;
      int align = 1;
      int flags = options_.sws_flags;

      if (pix_fmt == AV_PIX_FMT_NONE) pix_fmt = codec_ctx_->pix_fmt;
      if (width <= 0) width = codec_ctx_->width;
      if (height <= 0) height = codec_ctx_->height;
      if (flags == 0) flags = SWS_BICUBIC;

      sws_frame_ = av_frame_alloc();

      int bytes_n = av_image_get_buffer_size(pix_fmt, width, height, align);
      uint8_t *buffer = static_cast<uint8_t *>(
        av_malloc(bytes_n * sizeof(uint8_t)));
      av_image_fill_arrays(sws_frame_->data, sws_frame_->linesize, buffer,
        pix_fmt, width, height, align);

      sws_frame_->width = width;
      sws_frame_->height = height;

      sws_ctx_ = sws_getContext(
          codec_ctx_->width, codec_ctx_->height, codec_ctx_->pix_fmt,
          width, height, pix_fmt, flags, nullptr, nullptr, nullptr);
      if (sws_ctx_ == nullptr) throw StreamError("Get sws context fail");
    }

    sws_scale(sws_ctx_, frame_->data, frame_->linesize, 0, codec_ctx_->height,
      sws_frame_->data, sws_frame_->linesize);

    result = sws_frame_;
  }

  return result;
}

void StreamVideo::Flush() {
  if (codec_ctx_) {
    avcodec_flush_buffers(codec_ctx_);
  }
}

void StreamVideo::Free() {
  if (sws_frame_) {
    av_frame_free(&sws_frame_);
    sws_frame_ = nullptr;
  }
  if (sws_ctx_) {
    sws_freeContext(sws_ctx_);
    sws_ctx_ = nullptr;
  }
  if (frame_) {
    av_frame_free(&frame_);
    frame_ = nullptr;
  }
  if (codec_ctx_) {
    avcodec_close(codec_ctx_);
    avcodec_free_context(&codec_ctx_);
    codec_ctx_ = nullptr;
  }
}
