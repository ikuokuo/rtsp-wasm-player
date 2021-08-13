#pragma once

#include <memory>
#include <unordered_map>

#ifdef __cplusplus
extern "C" {
#endif

struct AVFormatContext;
struct AVStream;
struct AVPacket;
struct AVFrame;

#include <libavcodec/packet.h>
#include <libavutil/frame.h>

#ifdef __cplusplus
}
#endif

#include "stream_def.h"

class StreamSub {
 public:
  explicit StreamSub(AVStream *stream) : stream_(stream) {}
  virtual ~StreamSub() = default;

  AVStream *stream() const { return stream_; }

  virtual int GetIndex() = 0;
  virtual AVFrame *GetFrame(AVPacket *packet) = 0;

 protected:
  AVStream *stream_;
};

class Stream {
 public:
  using stream_sub_t = std::shared_ptr<StreamSub>;
  using stream_subs_t = std::unordered_map<AVMediaType, stream_sub_t>;

  Stream() noexcept;
  ~Stream() noexcept;

  bool IsOpen() const noexcept;
  void Open(const StreamOptions &options);

  AVPacket *GetPacket(bool unref = true);
  AVFrame *GetFrame(AVMediaType type, AVPacket *packet = nullptr,
    bool unref = false);
  void UnrefPacket();  // unref packet after get

  AVFrame *GetFrameVideo();

  void Close();

  StreamOptions GetOptions() const;
  stream_subs_t GetStreamSubs() const;
  stream_sub_t GetStreamSub(AVMediaType type) const;

 private:
  StreamOptions options_;
  bool is_open_;

  AVFormatContext *format_ctx_;
  AVPacket *packet_;

  stream_subs_t stream_subs_;
};
