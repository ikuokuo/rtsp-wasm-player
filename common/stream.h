#pragma once

#include <functional>
#include <memory>
#include <string>
#include <stdexcept>
#include <tuple>
#include <unordered_map>

#ifdef __cplusplus
extern "C" {
#endif

struct AVFormatContext;
struct AVPacket;
struct AVFrame;

#include <libavcodec/packet.h>
#include <libavutil/frame.h>

#ifdef __cplusplus
}
#endif

class StreamError : public std::exception {
 public:
  explicit StreamError(const std::string &what_arg) noexcept;
  explicit StreamError(int av_err) noexcept;
  const char *what() const noexcept;
 private:
  std::string what_message_;
};

class StreamSub {
 public:
  virtual ~StreamSub() = default;
  virtual int GetIndex() = 0;
  virtual AVFrame *GetFrame(AVPacket *packet) = 0;
};

class Stream {
 public:
  enum Method {
    METHOD_NONE,
    METHOD_NETWORK,
    METHOD_WEBCAM,
  };

  struct Options {
    Method method = METHOD_NONE;
    // if network
    //  rtsp://
    // if webcam
    //  v4l2: /dev/video0, ...
    std::string input_url;

    // avdevice options, only for webcam
    std::string input_format;  // v4l2, ...
    int width;
    int height;
    int framerate;
    AVPixelFormat pixel_format;

    // avformat options
    /**
     * max memory used for buffering real-time frames
     *
     * if webcam: value = width * height * framerate(15 if not set) * 2(bpp, bytes per pixel)
     *
     * @see libavformat/options_table.h
     */
    int64_t rtbufsize;

    // swscale options
    bool sws_enable;      // enable or not
    int sws_dst_width;    // src width if 0
    int sws_dst_height;   // src height if 0
    AVPixelFormat sws_dst_pix_fmt;
    int sws_flags;
  };

  Stream() noexcept;
  ~Stream() noexcept;

  bool IsOpen() const noexcept;
  void Open(const Options &options);

  AVPacket *GetPacket(bool unref = true);
  AVFrame *GetFrame(AVMediaType type, AVPacket *packet = nullptr,
    bool unref = false);
  void UnrefPacket();  // unref packet after get

  AVFrame *GetFrameVideo();

  void Close();

  static std::string GetPixFmtString(AVPixelFormat pix_fmt) noexcept;

 private:
  std::shared_ptr<StreamSub> GetStreamSub(AVMediaType type);

  Options options_;
  bool is_open_;

  AVFormatContext *format_ctx_;
  AVPacket *packet_;

  std::unordered_map<AVMediaType, std::shared_ptr<StreamSub>> stream_subs_;
};
