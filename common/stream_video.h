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
  StreamVideo(const Stream::Options &options, AVStream *stream);
  virtual ~StreamVideo();

  int GetIndex() override;
  AVFrame *GetFrame(AVPacket *packet) override;

  void Flush();
  void Free();

 private:
  Stream::Options options_;
  AVStream *stream_;

  AVCodecContext *codec_ctx_;
  AVFrame *frame_;

  SwsContext *sws_ctx_;
  AVFrame *sws_frame_;
};
