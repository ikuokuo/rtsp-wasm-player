#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

struct AVBSFContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;

#ifdef __cplusplus
}
#endif

class Stream;
struct StreamSub;

class StreamVideoEncoder;

enum StreamFilterType {
  STREAM_FILTER_NONE,
  STREAM_FILTER_VIDEO_BSF,
  STREAM_FILTER_VIDEO_ENC,
};

enum StreamFilterStatus {
  STREAM_FILTER_STATUS_OK,
  STREAM_FILTER_STATUS_AGAIN,
  STREAM_FILTER_STATUS_BREAK,
};

std::string StreamFilterTypeToString(StreamFilterType type);
StreamFilterType StreamFilterTypeFromString(const std::string &type);

struct StreamFilterOptions {
  StreamFilterType type = STREAM_FILTER_NONE;

  // bsf options
  std::string bsf_name = "";

  // encode options
  //  set if int > -1, str not empty
  std::string enc_name = "";
  int enc_bit_rate = -1;
  int enc_framerate = -1;
  int enc_gop_size = -1;
  int enc_max_b_frames = -1;
  int enc_qmin = -1;
  int enc_qmax = -1;
  int enc_thread_count = -1;
  std::map<std::string, std::string> enc_open_options{};

  std::string dec_name = "";
  int dec_thread_count = -1;
  //  1: FF_THREAD_FRAME, 2: FF_THREAD_SLICE
  int dec_thread_type = -1;
};

class StreamFilter {
 public:
  StreamFilter(const std::shared_ptr<StreamSub> &stream,
      const StreamFilterOptions &options);
  virtual ~StreamFilter();

  const StreamFilterOptions &GetOptions() const;

  virtual StreamFilterStatus SendPacket(AVPacket *pkt) = 0;
  virtual StreamFilterStatus RecvPacket(AVPacket *pkt) = 0;

 protected:
  std::shared_ptr<StreamSub> stream_;
  StreamFilterOptions options_;
};

// StreamFilterVideoBSF

class StreamFilterVideoBSF : public StreamFilter {
 public:
  StreamFilterVideoBSF(const std::shared_ptr<StreamSub> &stream,
      const StreamFilterOptions &options);
  ~StreamFilterVideoBSF() override;

  StreamFilterStatus SendPacket(AVPacket *pkt) override;
  StreamFilterStatus RecvPacket(AVPacket *pkt) override;

 private:
  AVBSFContext *bsf_ctx_;
};

// StreamFilterVideoEnc

class StreamVideoOp;

class StreamFilterVideoEnc : public StreamFilter {
 public:
  StreamFilterVideoEnc(const std::shared_ptr<StreamSub> &stream,
      const StreamFilterOptions &options);
  ~StreamFilterVideoEnc() override;

  StreamFilterStatus SendPacket(AVPacket *pkt) override;
  StreamFilterStatus RecvPacket(AVPacket *pkt) override;

 private:
  std::shared_ptr<StreamVideoOp> decoder_;
  std::shared_ptr<StreamVideoEncoder> encoder_;
  AVFrame *encode_frame_;
  int64_t encode_frame_pts_;
  std::chrono::system_clock::time_point encode_frame_timestamp_;
};
