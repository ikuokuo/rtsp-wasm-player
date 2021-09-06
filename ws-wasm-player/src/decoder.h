#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "common/media/stream_video.h"
#include "common/net/json.h"
#include "common/net/packet.h"

#include "frame.h"

class Decoder {
 public:
  using stream_ops_t =
      std::unordered_map<AVMediaType, std::shared_ptr<StreamOp>>;

  class StreamVideoOpContext : public StreamOpContext {
   public:
    explicit StreamVideoOpContext(AVCodecParameters *par)
      : codecpar_(par) {
    }
    ~StreamVideoOpContext() override {}

    AVCodecID GetAVCodecID() override {
      return codecpar_->codec_id;
    }
    void InitAVCodecContext(AVCodecContext *codec_ctx) override {
      int ret = avcodec_parameters_to_context(codec_ctx, codecpar_);
      if (ret < 0) throw StreamError(ret);
    }

   private:
    AVCodecParameters *codecpar_;
  };

  Decoder() {
    VLOG(2) << __func__;
  }
  ~Decoder() {
    VLOG(2) << __func__;
  }

  void Open(const std::string &stream_info) {
    VLOG(1) << __func__ << ": " << stream_info;
    info_ = net::to_stream_info(stream_info);

    for (auto &&e : info_.subs) {
      auto type = e.first;
      auto sub_info = e.second;
      switch (type) {
      case AVMEDIA_TYPE_VIDEO: {
        StreamVideoOptions options{};
        options.sws_enable = true;
        options.sws_dst_pix_fmt = AV_PIX_FMT_YUV420P;
        ops_[type] = std::make_shared<StreamVideoOp>(
            options,
            std::make_shared<Decoder::StreamVideoOpContext>(
                sub_info->codecpar));
      } break;
      default:
        LOG(WARNING) << "Stream[" << info_.id << "] "
            << "media type not support at present, type="
            << av_get_media_type_string(type);
        break;
      }
    }
  }

  // embind doesn't support pointers to primitive types
  //  https://stackoverflow.com/a/27364643
  std::shared_ptr<Frame> Decode(uintptr_t buf_p, int buf_size) {
    const uint8_t *buf = reinterpret_cast<uint8_t *>(buf_p);

    net::Data data;
    data.FromBytes(buf, buf_size);
    VLOG(1) << "decode packet type=" << av_get_media_type_string(data.type)
        << ", size=" << data.packet->size;

    try {
      auto op = ops_[data.type];
      auto frame = op->GetFrame(data.packet);
      if (frame == nullptr) {
        LOG(WARNING) << "decode frame is null, need new packets";
        return nullptr;
      }
      return std::make_shared<Frame>()->Alloc(data.type, frame);
    } catch (const StreamError &err) {
      LOG(ERROR) << err.what();
      return nullptr;
    }
  }

 private:
  StreamInfo info_;
  stream_ops_t ops_;
};
