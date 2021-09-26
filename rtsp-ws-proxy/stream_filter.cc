#include "stream_filter.h"

#include <string>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>

#ifdef __cplusplus
}
#endif

#include "common/media/stream.h"
#include "common/media/stream_video.h"
#include "common/util/log.h"
#include "common/util/throw_error.h"

#include "stream_video_encoder.h"

std::string StreamFilterTypeToString(StreamFilterType type) {
  switch (type) {
    case STREAM_FILTER_NONE:      return "none";
    case STREAM_FILTER_VIDEO_BSF: return "video_bsf";
    case STREAM_FILTER_VIDEO_ENC: return "video_enc";
    default: throw StreamError("StreamFilterType unknown");
  }
}

StreamFilterType StreamFilterTypeFromString(const std::string &type) {
  if (type == "none")       return STREAM_FILTER_NONE;
  if (type == "video_bsf")  return STREAM_FILTER_VIDEO_BSF;
  if (type == "video_enc")  return STREAM_FILTER_VIDEO_ENC;
  throw_error<StreamError>() << "StreamFilterType unknown: " << type;
  return STREAM_FILTER_NONE;
}

// StreamFilter

StreamFilter::StreamFilter(const std::shared_ptr<StreamSub> &stream,
    const StreamFilterOptions &options)
  : stream_(stream), options_(options) {
}

StreamFilter::~StreamFilter() {
}

const StreamFilterOptions &StreamFilter::GetOptions() const {
  return options_;
}

// StreamFilterVideoBSF

StreamFilterVideoBSF::StreamFilterVideoBSF(
    const std::shared_ptr<StreamSub> &stream,
    const StreamFilterOptions &options)
  : StreamFilter(stream, options), bsf_ctx_(nullptr) {
  VLOG(2) << __func__;
  LOG_IF(FATAL, options.type != STREAM_FILTER_VIDEO_BSF);
}

StreamFilterVideoBSF::~StreamFilterVideoBSF() {
  VLOG(2) << __func__;
  if (bsf_ctx_) {
    av_bsf_free(&bsf_ctx_);
    bsf_ctx_ = nullptr;
  }
}

StreamFilterStatus StreamFilterVideoBSF::SendPacket(AVPacket *pkt) {
  if (bsf_ctx_ == nullptr) {
    auto stream = stream_->stream;
    auto codec_id = stream->codecpar->codec_id;

    // ./configure --list-bsfs , ffmpeg -hide_banner -bsfs
    std::string bsf_name;
    if (!options_.bsf_name.empty()) {
      bsf_name = options_.bsf_name;
    } else {
      switch (codec_id) {
      case AV_CODEC_ID_H264: bsf_name = "h264_mp4toannexb"; break;
      case AV_CODEC_ID_HEVC: bsf_name = "hevc_mp4toannexb"; break;
      case AV_CODEC_ID_RAWVIDEO: bsf_name = "null"; break;
      default:
        throw_error<StreamError>() << "BSF name not accept, id=" << codec_id;
      }
    }

    auto bsf = av_bsf_get_by_name(bsf_name.c_str());
    if (bsf == nullptr) {
      throw_error<StreamError>() << "BSF not found, name=" << bsf_name;
    }

    int ret = av_bsf_alloc(bsf, &bsf_ctx_);
    if (ret < 0) throw StreamError(ret);

    ret = avcodec_parameters_copy(bsf_ctx_->par_in, stream->codecpar);
    if (ret < 0) throw StreamError(ret);

    ret = av_bsf_init(bsf_ctx_);
    if (ret != 0) throw StreamError(ret);
  }

  int ret = av_bsf_send_packet(bsf_ctx_, pkt);
  if (ret != 0) {
    if (ret == AVERROR(EAGAIN)) {
      return STREAM_FILTER_STATUS_BREAK;
    } else {
      throw StreamError(ret);
    }
  }
  return STREAM_FILTER_STATUS_OK;
}

StreamFilterStatus StreamFilterVideoBSF::RecvPacket(AVPacket *pkt) {
  LOG_IF(FATAL, bsf_ctx_ == nullptr);
  int ret = av_bsf_receive_packet(bsf_ctx_, pkt);
  if (ret != 0) {
    if (ret == AVERROR(EAGAIN)) {
      return STREAM_FILTER_STATUS_BREAK;
    } else {
      throw StreamError(ret);
    }
  }
  return STREAM_FILTER_STATUS_OK;
}

// StreamFilterVideoEnc

StreamFilterVideoEnc::StreamFilterVideoEnc(
    const std::shared_ptr<StreamSub> &stream,
    const StreamFilterOptions &options)
  : StreamFilter(stream, options), decoder_(nullptr), encoder_(nullptr),
    encode_frame_(nullptr), encode_frame_pts_(0) {
  VLOG(2) << __func__;
  LOG_IF(FATAL, options.type != STREAM_FILTER_VIDEO_ENC);
}

StreamFilterVideoEnc::~StreamFilterVideoEnc() {
  VLOG(2) << __func__;
  if (encode_frame_) {
    av_frame_free(&encode_frame_);
    encode_frame_ = nullptr;
  }
}

StreamFilterStatus StreamFilterVideoEnc::SendPacket(AVPacket *pkt) {
  // decode

  if (decoder_ == nullptr) {
    StreamVideoOptions options{};
    if (!options_.dec_name.empty())
      options.dec_name = options_.dec_name;
    if (options_.dec_thread_count > -1)
      options.dec_thread_count = options_.dec_thread_count;
    if (options_.dec_thread_type > -1)
      options.dec_thread_type = options_.dec_thread_type;
    // NVIDIA Video Codec SDK
    //  https://developer.nvidia.com/nvidia-video-codec-sdk
    // NVIDIA FFmpeg Transcoding Guide
    //  https://developer.nvidia.com/blog/nvidia-ffmpeg-transcoding-guide/
    // nvenc
    //  YUVJ420P not support, need convert to YUV420P, then encode to H264/HEVC
    options.sws_enable = true;
    options.sws_dst_pix_fmt = AV_PIX_FMT_YUV420P;
    decoder_ = std::make_shared<StreamVideoOp>(
        options,
        std::make_shared<StreamVideoOpContext>(stream_->stream));
  }

  auto frame = decoder_->GetFrame(pkt);
  if (frame == nullptr)
    return STREAM_FILTER_STATUS_BREAK;

  // encode

  if (encoder_ == nullptr) {
    StreamVideoEncodeOptions options{};
    options.codec_name = options_.enc_name;
    options.codec_bit_rate = options_.enc_bit_rate;
    options.codec_width = frame->width;
    options.codec_height = frame->height;
    options.codec_framerate = options_.enc_framerate;
    options.codec_pix_fmt = AV_PIX_FMT_YUV420P;
    options.codec_gop_size = options_.enc_gop_size;
    options.codec_max_b_frames = options_.enc_max_b_frames;
    options.codec_qmin = options_.enc_qmin;
    options.codec_qmax = options_.enc_qmax;
    options.codec_thread_count = options_.enc_thread_count;
    options.open_options = options_.enc_open_options;

    encoder_ = std::make_shared<StreamVideoEncoder>(options);

    avcodec_parameters_from_context(stream_->info->codecpar,
        encoder_->GetCodecContext());

    encode_frame_ = av_frame_alloc();
    encode_frame_->width  = options.codec_width;
    encode_frame_->height = options.codec_height;
    encode_frame_->format = options.codec_pix_fmt;

    int ret = av_frame_get_buffer(encode_frame_, 0);
    if (ret < 0) {
      LOG(ERROR) << "FR alloc frame fail";
      throw StreamError(ret);
    }
  }

  if (options_.enc_framerate > 0) {
    // drop frame according to the framerate
    auto t_now = std::chrono::system_clock::now();
    auto t_interval = std::chrono::duration_cast<std::chrono::milliseconds>(
        t_now - encode_frame_timestamp_).count();
    if (t_interval < (1000 / options_.enc_framerate)) {
      return STREAM_FILTER_STATUS_BREAK;
    }
    encode_frame_timestamp_ = t_now;
  }

  /*{
    int i = encode_frame_pts_;
    int x, y;
    av_frame_make_writable(encode_frame_);

    for (y = 0; y < encode_frame_->height; y++) {
      for (x = 0; x < encode_frame_->width; x++) {
        encode_frame_->data[0][y * encode_frame_->linesize[0] + x] =
            x + y + i * 3;
      }
    }
    for (y = 0; y < encode_frame_->height/2; y++) {
      for (x = 0; x < encode_frame_->width/2; x++) {
        encode_frame_->data[1][y * encode_frame_->linesize[1] + x] =
            128 + y + i * 2;
        encode_frame_->data[2][y * encode_frame_->linesize[2] + x] =
            64 + x + i * 5;
      }
    }
  }*/
  // av_frame_copy(encode_frame_, frame);  // not work
  {
    av_frame_make_writable(encode_frame_);
    const uint8_t *src_data[4];
    memcpy(src_data, frame->data, sizeof(src_data));
    av_image_copy(encode_frame_->data, encode_frame_->linesize,
                  src_data, frame->linesize,
                  static_cast<AVPixelFormat>(encode_frame_->format),
                  frame->width, frame->height);
  }
  encode_frame_->pts = encode_frame_pts_++;

  int ret = encoder_->Send(encode_frame_);
  if (ret < 0) throw StreamError(ret);

  return STREAM_FILTER_STATUS_OK;
}

StreamFilterStatus StreamFilterVideoEnc::RecvPacket(AVPacket *pkt) {
  LOG_IF(FATAL, encoder_ == nullptr);
  int ret = encoder_->Recv(pkt);
  if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
    return STREAM_FILTER_STATUS_BREAK;  // recv fail,
  } else if (ret < 0) {
    throw StreamError(ret);
  }
  return STREAM_FILTER_STATUS_AGAIN;  // recv ok, recv again
}
