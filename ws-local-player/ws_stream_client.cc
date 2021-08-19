#include "ws_stream_client.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>

#ifdef __cplusplus
}
#endif

#include <glog/logging.h>

#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>

#include "common/media/stream_video.h"

namespace asio = boost::asio;

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
    const StreamInfo &info)
  : WsClient(options), info_(info), ui_ok_(false), ui_(nullptr),
    packet_(av_packet_alloc()) {
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
  av_packet_free(&packet_);
}

void WsStreamClient::Run() {
  asio::io_context ioc;

  boost::asio::spawn(ioc, std::bind(
      &WsStreamClient::DoSession,
      this,
      std::ref(ioc),
      std::placeholders::_1));

  std::vector<std::thread> v;
  v.emplace_back([&ioc]() {
    ioc.run();
  });

  {
    int n = 5;
    while (--n) {
      std::unique_lock<std::mutex> lock(ui_mutex_);
      auto ok = ui_cond_.wait_for(lock,
          std::chrono::seconds(1),
          [this]() { return ui_ok_; });
      if (ok) break;
    }
    if (ui_ok_) {
      {
        std::lock_guard<std::mutex> _(ui_mutex_);
        ui_ = std::make_shared<GlfwFrame>();
      }
      LOG(INFO) << "Stream[" << info_.id << "] ui run";
      ui_->Run(ui_params_);
      LOG(INFO) << "Stream[" << info_.id << "] ui over";
    }
  }

  ioc.stop();
  for (auto &t : v)
    t.join();
}

bool WsStreamClient::OnRead(boost::beast::flat_buffer *buffer) {
  // type size data, ...
  // 1    4    -
  auto data = buffer->data();
  auto bytes = reinterpret_cast<char *>(data.data());

  auto type = static_cast<AVMediaType>(*(bytes));
  auto size = ((*(bytes + 1) & 0xff) << 24) |
      ((*(bytes + 2) & 0xff) << 16) |
      ((*(bytes + 3) & 0xff) << 8) |
      ((*(bytes + 4) & 0xff));
  buffer->consume(5);

  if (type == AVMEDIA_TYPE_VIDEO) {
    auto data = static_cast<uint8_t *>(av_malloc(size));
    if (data) {
      std::copy(bytes, bytes + size, data);
      av_packet_from_data(packet_, data, size);
      OnPacket(type, packet_);
      av_packet_unref(packet_);
    } else {
      LOG(ERROR) << "av_malloc fail, size=" << size;
    }
  }
  buffer->consume(size);
  return true;
}

void WsStreamClient::OnPacket(const AVMediaType &type, AVPacket *packet) {
  VLOG(1) << "packet type=" << type << ", size=" << packet->size;
  auto op = ops_[type];
  auto frame = op->GetFrame(packet);
  if (frame == nullptr) return;

  if (type == AVMEDIA_TYPE_VIDEO) {
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
}
