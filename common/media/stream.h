#pragma once

#include <memory>
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

#include "stream_def.h"

class StreamSub {
 public:
  virtual ~StreamSub() = default;
  virtual int GetIndex() = 0;
  virtual AVFrame *GetFrame(AVPacket *packet) = 0;
};

class Stream {
 public:
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

 private:
  std::shared_ptr<StreamSub> GetStreamSub(AVMediaType type);

  StreamOptions options_;
  bool is_open_;

  AVFormatContext *format_ctx_;
  AVPacket *packet_;

  std::unordered_map<AVMediaType, std::shared_ptr<StreamSub>> stream_subs_;
};
