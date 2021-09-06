#pragma once

#include <memory>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/frame.h>
#include <libavutil/imgutils.h>

#ifdef __cplusplus
}
#endif

#include <emscripten/val.h>

#include "common/util/log.h"

class Frame : public std::enable_shared_from_this<Frame> {
 public:
  Frame()
    : type_(AVMEDIA_TYPE_UNKNOWN), frame_(nullptr) {
    VLOG(2) << __func__;
  }
  ~Frame() {
    VLOG(2) << __func__;
  }

  std::shared_ptr<Frame> Alloc(AVMediaType type, AVFrame *frame) {
    type_ = type;
    frame_ = frame;
    // frame_ = av_frame_clone(frame);
    // if (type_ == AVMEDIA_TYPE_VIDEO) {
    //   VLOG(1) << " [v] frame size=" << width() << "x" << height()
    //       << ", fmt: " << format();
    // }
    return shared_from_this();
  }

  void Free() {
    type_ = AVMEDIA_TYPE_UNKNOWN;
    frame_ = nullptr;
    // if (frame_ != nullptr) {
    //   av_frame_free(&frame_);
    //   frame_ = nullptr;
    // }
  }

  int type() const { return type_; }

  uint8_t *data() const { return frame_->data[0]; }
  int linesize() const { return frame_->linesize[0]; }
  int width() const { return frame_->width; }
  int height() const { return frame_->height; }
  int format() const { return frame_->format; }
  int64_t pts() const { return frame_->pts; }

  int data_ptr() const { return (int)(frame_->data[0]); }  // NOLINT
  int size() const {
    return av_image_get_buffer_size(
        AV_PIX_FMT_YUV420P, frame_->width, frame_->height, 1);
  }
  emscripten::val GetBytes() {
    return emscripten::val(
        emscripten::typed_memory_view(size(), frame_->data[0]));
  }

 private:
  int type_;
  AVFrame *frame_;
};
