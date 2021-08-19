#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct AVStream;
struct AVPacket;
struct AVFrame;
struct AVCodecContext;
struct SwsContext;

#ifdef __cplusplus
}
#endif

#include "stream.h"

class StreamVideo : public StreamSub {
 public:
  StreamVideo(const StreamVideoOptions &options, AVStream *stream);
  virtual ~StreamVideo();

  AVFrame *GetFrame(AVPacket *packet) override;

  void Flush();
  void Free();

 private:
  StreamVideoOptions options_;

  AVCodecContext *codec_ctx_;
  AVFrame *frame_;

  SwsContext *sws_ctx_;
  AVFrame *sws_frame_;
};
