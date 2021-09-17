#include "ws_stream_client.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>

#ifdef __cplusplus
}
#endif

#include <algorithm>
#include <chrono>
#include <vector>

#include "common/media/stream_video.h"
#include "common/net/packet.h"
#include "common/util/log.h"
#include "common/util/logext.h"

namespace client {

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

}  // namespace client

WsStreamClient::WsStreamClient(
    asio::io_context &ioc,
    const WsStreamClientOptions &opts)
  : WsClient<std::vector<uint8_t>>(ioc, opts.ws),
    info_(opts.stream_info),
    ui_wait_secs_(opts.ui_wait_secs), ui_ok_(false), ui_(nullptr),
    on_ui_exit_(opts.ui_exit_func) {
  if (ui_wait_secs_ <= 0) ui_wait_secs_ = 10;

  for (auto &&e : info_.subs) {
    auto type = e.first;
    auto sub_info = e.second;
    switch (type) {
    case AVMEDIA_TYPE_VIDEO: {
      StreamVideoOptions options{};
      options.codec_thread_count = opts.codec_thread_count;
      options.codec_thread_type = opts.codec_thread_type;
      options.sws_enable = true;
      if (sub_info->codecpar->format == AV_PIX_FMT_YUVJ420P || (
          sub_info->codecpar->format == AV_PIX_FMT_YUV420P &&
          sub_info->codecpar->color_range == AVCOL_RANGE_JPEG)) {
        options.sws_dst_pix_fmt = AV_PIX_FMT_YUVJ420P;
      } else {
        options.sws_dst_pix_fmt = AV_PIX_FMT_YUV420P;
      }
      ops_[type] = std::make_shared<StreamVideoOp>(
          options,
          std::make_shared<client::StreamVideoOpContext>(
              sub_info->codecpar));
    } break;
    default:
      LOG(WARNING) << "Stream[" << info_.id << "] "
          << "media type not support at present, type="
          << av_get_media_type_string(type);
      break;
    }
  }

  ui_thread_ = std::thread(std::bind(&WsStreamClient::Run, this));
}

WsStreamClient::~WsStreamClient() {
  ui_thread_.join();
}

void WsStreamClient::Run() {
  int ui_wait_ms = 2000;
  int n = ui_wait_secs_ * 1000 / ui_wait_ms;
  while (n--) {
    std::unique_lock<std::mutex> lock(ui_mutex_);
    auto ok = ui_cond_.wait_for(lock,
        std::chrono::milliseconds(ui_wait_ms),
        [this]() { return ui_ok_; });
    if (ok) break;
    LOG_IF(INFO, n) << "Stream[" << info_.id << "] wait ...";
  }
  if (ui_ok_) {
    {
      std::lock_guard<std::mutex> _(ui_mutex_);
      ui_ = std::make_shared<GlfwFrame>();
    }
    LOG(INFO) << "Stream[" << info_.id << "] ui run";
    ui_->Run(ui_params_);
    LOG(INFO) << "Stream[" << info_.id << "] ui over";
  } else {
    LOG(WARNING) << "Stream[" << info_.id << "] wait timeout";
  }
  if (on_ui_exit_) on_ui_exit_();
}

void WsStreamClient::OnEventRecv(
    beast::flat_buffer &buffer, std::size_t bytes_n) {
  auto t = logext::TimeRecord::Create("WsStreamClient::OnEventRecv");
  WsClient<std::vector<uint8_t>>::OnEventRecv(buffer, bytes_n);

  t->Beg("FromBytes");
  net::Data data;
  auto buf = buffer.data();
  data.FromBytes(buf);
  buffer.consume(buf.size());
  t->End();

  VLOG(2) << "buf_n=" << buf.size() << ", bytes_n=" << bytes_n
      << ", type=" << data.type << ", packet_n=" << data.packet->size;

  auto op = ops_[data.type];
  t->Beg("GetFrame");
  auto frame = op->GetFrame(data.packet);
  t->End();
  if (frame == nullptr) return;

  if (data.type == AVMEDIA_TYPE_VIDEO) {
    VLOG(1) << " [v] frame size=" << frame->width << "x" << frame->height
        << ", fmt: " << frame->format;
    if (ui_ok_) {
      std::lock_guard<std::mutex> _(ui_mutex_);
      ui_->Update(frame);
    } else {
      std::unique_lock<std::mutex> lock(ui_mutex_);
      ui_ok_ = true;
      ui_params_ = {frame->width, frame->height, info_.id};
      ui_cond_.notify_one();
    }
  }

  VLOG(2) << t->Log();
}
