#include "stream_def.h"

#include <utility>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/error.h>
#include <libavutil/pixdesc.h>

#ifdef __cplusplus
}
#endif

std::string StreamPixFmtString(AVPixelFormat pix_fmt) noexcept {
  auto desc = av_pix_fmt_desc_get(pix_fmt);
  return desc->name;
}

// StreamError

StreamError::StreamError(const std::string &what_arg) noexcept
  : what_message_(std::move(what_arg)) {
}

StreamError::StreamError(int av_err) noexcept {
  char str[AV_ERROR_MAX_STRING_SIZE];
  av_make_error_string(&str[0], AV_ERROR_MAX_STRING_SIZE, av_err);
  what_message_ = str;
}

const char *StreamError::what() const noexcept {
  return what_message_.c_str();
}
