#pragma once

#include <string>
#include <stdexcept>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/common.h>
#include <libavutil/pixfmt.h>

#ifdef __cplusplus
}
#endif

enum StreamMethod {
  STREAM_METHOD_NONE,
  STREAM_METHOD_FILE,
  STREAM_METHOD_NETWORK,
  STREAM_METHOD_WEBCAM,
};

struct StreamOptions {
  StreamMethod method = STREAM_METHOD_NONE;
  // if network
  //  rtsp://
  // if webcam
  //  v4l2: /dev/video0, ...
  std::string input_url;

  // avdevice options, only for webcam
  std::string input_format;  // v4l2, ...
  int width     = 0;  // set if > 0
  int height    = 0;  // set if > 0
  int framerate = 0;  // set if > 0
  AVPixelFormat pixel_format = AV_PIX_FMT_NONE;  // set if != NONE

  // avformat options
  /**
   * max memory used for buffering real-time frames, set if > 0
   *
   * if webcam: value = width * height * framerate(15 if not set) * 2(bpp, bytes per pixel)
   *
   * @see libavformat/options_table.h
   */
  int64_t rtbufsize = 0;

  // swscale options
  bool sws_enable     = false;  // enable or not
  int sws_dst_width   = 0;      // src width if <= 0
  int sws_dst_height  = 0;      // src width if <= 0
  AVPixelFormat sws_dst_pix_fmt = AV_PIX_FMT_NONE;  // src pix_fmt if NONE
  int sws_flags = 0;  // SWS_BICUBIC if 0
};

enum StreamErrorCode {
  STREAM_ERROR_ANY = -0xA0000000,
  // AVERROR
  STREAM_ERROR_EOF = AVERROR_EOF,
};

class StreamError : public std::exception {
 public:
  explicit StreamError(int av_err) noexcept;
  explicit StreamError(const std::string &what_arg) noexcept;
  StreamError(int code, const std::string &what_arg) noexcept;

  int code() const noexcept;
  const char *what() const noexcept;

 private:
  int code_;
  std::string what_message_;
};

// to/from string

std::string StreamMethodToString(StreamMethod method);
StreamMethod StreamMethodFromString(const std::string &method);

std::string PixelFormatToString(AVPixelFormat pix_fmt);
AVPixelFormat PixelFormatFromString(const std::string &pix_fmt);
