#include "stream_player.h"

#include "common/media/stream_video.h"
#include "common/util/throw_error.h"

StreamPlayer::StreamPlayer(const std::string &id)
  : id_(id), ui_ok_(false), ui_(nullptr) {
}

StreamPlayer::~StreamPlayer() {
}

void StreamPlayer::Start() {
  ui_thread_ = std::thread([this]() {
    while (true) {
      std::unique_lock<std::mutex> lock(ui_mutex_);
      auto ok = ui_cond_.wait_for(lock,
          std::chrono::seconds(1),
          [this]() { return ui_ok_; });
      if (ok) break;
    }
    {
      std::lock_guard<std::mutex> _(ui_mutex_);
      ui_ = std::make_shared<GlfwFrame>();
    }
    LOG(INFO) << "Stream[" << id_ << "] ui run";
    ui_->Run(ui_params_);
    LOG(INFO) << "Stream[" << id_ << "] ui over";
    {
      std::lock_guard<std::mutex> _(ui_mutex_);
      ui_ = nullptr;
    }
  });
}

void StreamPlayer::Stop() {
  if (ui_thread_.joinable()) {
    ui_thread_.join();
  }
}

void StreamPlayer::Send(
    const std::string &id,
    const std::shared_ptr<Stream> &stream,
    const AVMediaType &type,
    AVPacket *packet) {
  if (id != id_) return;

  if (ops_.find(type) == ops_.end()) {
    auto sub = stream->GetStreamSub(type);
    switch (type) {
      case AVMEDIA_TYPE_VIDEO: {
        StreamVideoOptions options{};
        options.sws_enable = true;
        options.sws_dst_pix_fmt = AV_PIX_FMT_YUV420P;
        ops_[type] = std::make_shared<StreamVideoOp>(options,
            std::make_shared<StreamVideoOpContext>(sub.stream));
      } break;
      default: throw_error<StreamError>() << "Stream[" << id_ << "] "
          << "media type not support at present, type="
          << av_get_media_type_string(type);
    }
  }

  auto op = ops_[type];
  auto frame = op->GetFrame(packet);
  if (frame == nullptr) return;

  if (type == AVMEDIA_TYPE_VIDEO) {
    VLOG(1) << "Stream[" << id_ << "][v] frame size=" << frame->width << "x"
        << frame->height << ", fmt: " << frame->format;
    if (ui_ok_) {
      if (ui_) {
        std::lock_guard<std::mutex> _(ui_mutex_);
        ui_->Update(frame);
      }
    } else {
      std::unique_lock<std::mutex> lock(ui_mutex_);
      ui_ok_ = true;
      ui_params_ = {frame->width, frame->height, id_};
      ui_cond_.notify_one();
    }
  }
}
