#pragma once

#include <functional>
#include <map>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavutil/frame.h>

#ifdef __cplusplus
}
#endif

struct StreamVideoEncodeOptions {
  /**
   * codec options, set if int > -1, str not empty
   *
   * YouTube / Recommended upload encoding settings / Video codec: H.264
   *  https://support.google.com/youtube/answer/1722171?hl=en#zippy=%2Cvideo-codec-h
   *
   * @see libavcodec/avcodec.h
   */
  std::string codec_name  = "h264_nvenc";
  int codec_bit_rate      = 400000;
  int codec_width         = 352;
  int codec_height        = 288;
  int codec_framerate     = 10;
  AVPixelFormat codec_pix_fmt = AV_PIX_FMT_YUV420P;
  int codec_gop_size      = 10;
  int codec_max_b_frames  = 1;
  int codec_qmin          = -1;
  int codec_qmax          = -1;
  int codec_thread_count  = -1;

  /**
   * open options
   *  ffmpeg -h encoder=h264_nvenc
   *   {{"preset", "slow"}, {"zerolatency", "1"}, {"rc", "vbr"}}
   *  ffmpeg -h encoder=libx264
   *   {{"preset", "slow"}, {"tune", "zerolatency"}, {"nal-hrd", "vbr"}, {"crf", "25"}}
   */
  std::map<std::string, std::string> open_options;
};

class StreamVideoEncoder {
 public:
  explicit StreamVideoEncoder(const StreamVideoEncodeOptions &options);
  ~StreamVideoEncoder();

  AVCodecContext *GetCodecContext();

  int Send(AVFrame *frame);
  int Recv(AVPacket *packet);

  void Encode(AVFrame *frame, std::function<void(AVPacket *)> on_recv);

  int Flush();

 private:
  void Init();
  void Free();

  StreamVideoEncodeOptions options_;
  AVCodecContext *codec_ctx_;
  AVPacket *packet_;
};
