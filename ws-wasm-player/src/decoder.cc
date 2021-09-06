#include <emscripten/bind.h>
#include <emscripten/val.h>
#ifndef NDEBUG
#include <sanitizer/lsan_interface.h>
#endif

#include <memory>
#include <string>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavutil/imgutils.h>

#ifdef __cplusplus
}
#endif

#include "common/media/stream_video.h"
#include "common/net/json.h"
#include "common/net/packet.h"
#include "common/util/log.h"

using namespace emscripten;  // NOLINT

struct Log {
  static void set_prefix(bool b) { UTIL_LOG_PREFIX = b; }
  static void set_minlevel(int n) { UTIL_LOG_MINLEVEL = n; }
  static void set_v(int n) { UTIL_LOG_V = n; }
};

class Frame : public std::enable_shared_from_this<Frame> {
 public:
  Frame()
    : type_(AVMEDIA_TYPE_UNKNOWN), frame_(nullptr) {
    VLOG(2) << __func__;
  }
  ~Frame() {
    VLOG(2) << __func__;
  }

  std::shared_ptr<Frame> Alloc(AVMediaType type, AVFrame *frame) {
    type_ = type;
    frame_ = frame;
    // frame_ = av_frame_clone(frame);
    // if (type_ == AVMEDIA_TYPE_VIDEO) {
    //   VLOG(1) << " [v] frame size=" << width() << "x" << height()
    //       << ", fmt: " << format();
    // }
    return shared_from_this();
  }

  void Free() {
    type_ = AVMEDIA_TYPE_UNKNOWN;
    frame_ = nullptr;
    // if (frame_ != nullptr) {
    //   av_frame_free(&frame_);
    //   frame_ = nullptr;
    // }
  }

  int type() const { return type_; }

  int data() const { return (int)(frame_->data[0]); }  // NOLINT
  int linesize() const { return frame_->linesize[0]; }
  int width() const { return frame_->width; }
  int height() const { return frame_->height; }
  int format() const { return frame_->format; }
  int64_t pts() const { return frame_->pts; }

  int size() const {
    return av_image_get_buffer_size(
        AV_PIX_FMT_YUV420P, frame_->width, frame_->height, 1);
  }
  val getBytes() {
    return val(typed_memory_view(size(), frame_->data[0]));
  }

 private:
  int type_;
  AVFrame *frame_;
};

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

EMSCRIPTEN_BINDINGS(decoder) {
#ifndef NDEBUG
  function("DoLeakCheck", &__lsan_do_recoverable_leak_check);
#endif

  class_<Log>("Log")
    .class_function("set_prefix", &Log::set_prefix)
    .class_function("set_minlevel", &Log::set_minlevel)
    .class_function("set_v", &Log::set_v);

  class_<Frame>("Frame")
    // .smart_ptr<std::shared_ptr<Frame>>("shared_ptr<Frame>")
    .smart_ptr_constructor("Frame", &std::make_shared<Frame>)
    .property("type", &Frame::type)
    .property("data", &Frame::data)
    .property("linesize", &Frame::linesize)
    .property("width", &Frame::width)
    .property("height", &Frame::height)
    .property("format", &Frame::format)
    .property("pts", &Frame::pts)
    .property("size", &Frame::size)
    .function("getBytes", &Frame::getBytes);

  class_<Decoder>("Decoder")
    .constructor<>()
    .function("Open", &Decoder::Open)
    .function("Decode", &Decoder::Decode, allow_raw_pointers());
}
