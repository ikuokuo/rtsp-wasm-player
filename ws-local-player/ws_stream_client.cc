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
#include <thread>
#include <vector>

#include "common/media/stream_video.h"
#include "common/net/packet.h"
#include "common/util/log.h"

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
    const WsClientOptions &options,
    const StreamInfo &info,
    int ui_wait_secs)
  : WsClient(options), info_(info), ui_wait_secs_(ui_wait_secs),
    ui_ok_(false), ui_(nullptr) {
  if (ui_wait_secs_ <= 0) ui_wait_secs_ = 10;

  for (auto &&e : info.subs) {
    auto type = e.first;
    auto sub_info = e.second;
    switch (type) {
    case AVMEDIA_TYPE_VIDEO: {
      StreamVideoOptions options{};
      options.sws_enable = true;
      options.sws_dst_pix_fmt = AV_PIX_FMT_YUV420P;
      ops_[type] = std::make_shared<StreamVideoOp>(
          options,
          std::make_shared<client::StreamVideoOpContext>(
              sub_info->codecpar));
    } break;
    default:
      LOG(WARNING) << "Stream[" << info.id << "] "
          << "media type not support at present, type="
          << av_get_media_type_string(type);
      break;
    }
  }
}

WsStreamClient::~WsStreamClient() {
}

void WsStreamClient::Run() {
  asio::io_context ioc;

  asio::spawn(ioc, std::bind(
      &WsStreamClient::DoSession,
      this,
      std::ref(ioc),
      std::placeholders::_1));

  std::vector<std::thread> v;
  v.emplace_back([&ioc]() {
    ioc.run();
  });

  {
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
  }

  ioc.stop();
  for (auto &t : v)
    t.join();
}

bool WsStreamClient::OnRead(beast::flat_buffer *buffer) {
  net::Data data;
  auto buf = buffer->data();
  data.FromBytes(buf);
  buffer->consume(buf.size());

  VLOG(1) << "packet type=" << data.type << ", size=" << data.packet->size;

  auto op = ops_[data.type];
  auto frame = op->GetFrame(data.packet);
  if (frame == nullptr) return true;

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
  return true;
}
