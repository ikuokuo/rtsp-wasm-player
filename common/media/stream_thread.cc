#include "stream_thread.h"

#include "stream.h"
#include "common/util/rate.h"

StreamThread::StreamThread(std::initializer_list<AVMediaType> types,
    bool loop_on_eof)
  : get_types_(types), loop_on_eof_(loop_on_eof),
    frequency_(20), event_cb_(nullptr), running_cb_(nullptr),
    is_running_(false) {
}

StreamThread::~StreamThread() {
  Stop();
}

bool StreamThread::IsRunning() const {
  return is_running_;
}

void StreamThread::SetEventCallback(event_callback_t cb) {
  event_cb_ = cb;
}

void StreamThread::SetRunningCallback(running_callback_t cb) {
  running_cb_ = cb;
}

void StreamThread::Start(const StreamOptions &options, int frequency) {
  if (frequency <= 0) throw StreamError("Process frequency must > 0");
  if (is_running_) return;
  is_running_ = true;
  options_ = options;
  frequency_ = frequency;
  thread_ = std::thread(&StreamThread::Run, this);
}

void StreamThread::Stop() {
  if (!is_running_) return;
  is_running_ = false;
  if (thread_.joinable()) {
    thread_.join();
  }
}

void StreamThread::Run() {
  bool loop = false;
  try {
    auto stream = std::make_shared<Stream>();

    DispatchEvent<StreamEvent>(STREAM_EVENT_OPEN, stream);
    stream->Open(options_);
    DispatchEvent<StreamEvent>(STREAM_EVENT_OPENED, stream);

    Rate rate(frequency_);
    while (is_running_) {
      if (running_cb_) {
        running_cb_(shared_from_this(), stream);
        rate.Sleep();
        continue;
      }

      auto packet = stream->GetPacket(false);
      DispatchEvent<StreamPacketEvent>(stream, packet);

      for (auto &&type : get_types_) {
        auto frame = stream->GetFrame(type, packet, false);
        if (frame != nullptr) {
          DispatchEvent<StreamFrameEvent>(stream, type, frame);
        }
      }

      stream->UnrefPacket();
      rate.Sleep();
    }

    DispatchEvent<StreamEvent>(STREAM_EVENT_CLOSE, stream);
    stream->Close();
    DispatchEvent<StreamEvent>(STREAM_EVENT_CLOSED, stream);
  } catch (const StreamError &err) {
    if (loop_on_eof_ && err.code() == STREAM_ERROR_EOF) {
      DispatchEvent<StreamEvent>(STREAM_EVENT_LOOP, nullptr);
      loop = true;
    } else {
      DispatchEvent<StreamErrorEvent>(nullptr, err);
    }
  }

  if (loop) Run();
}

void StreamThread::DispatchEvent(std::shared_ptr<StreamEvent> e) {
  if (event_cb_ != nullptr) {
    event_cb_(e);
  }
}
