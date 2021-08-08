#include "stream_thread.h"

#include "stream.h"
#include "common/util/rate.h"

StreamThread::StreamThread()
  : frequency_(20), event_cb_(nullptr), is_running_(false) {
}

StreamThread::~StreamThread() {
  Stop();
}

bool StreamThread::IsRunning() const {
  return is_running_;
}

void StreamThread::Start(const StreamOptions &options, int frequency) {
  if (frequency <= 0) throw StreamError("Process frequency must > 0");
  if (is_running_) return;
  is_running_ = true;
  options_ = options;
  frequency_ = frequency;
  thread_ = std::thread(&StreamThread::Run, this);
}

void StreamThread::SetEventCallback(event_callback_t cb) {
  event_cb_ = cb;
}

void StreamThread::Stop() {
  if (!is_running_) return;
  is_running_ = false;
  if (thread_.joinable()) {
    thread_.join();
  }
}

void StreamThread::Run() {
  try {
    Stream stream;

    DispatchEvent<StreamEvent>(STREAM_EVENT_OPEN);
    stream.Open(options_);
    DispatchEvent<StreamEvent>(STREAM_EVENT_OPENED);

    Rate rate(frequency_);
    while (is_running_) {
      auto packet = stream.GetPacket(false);
      DispatchEvent<StreamPacketEvent>(packet);

      auto frame = stream.GetFrame(AVMEDIA_TYPE_VIDEO, packet, false);
      if (frame == nullptr) {
        // could handle more events, such as EOF
      } else {
        DispatchEvent<StreamFrameEvent>(AVMEDIA_TYPE_VIDEO, frame);
      }

      stream.UnrefPacket();

      rate.Sleep();
    }

    DispatchEvent<StreamEvent>(STREAM_EVENT_CLOSE);
    stream.Close();
    DispatchEvent<StreamEvent>(STREAM_EVENT_CLOSED);
  } catch (const StreamError &err) {
    DispatchEvent<StreamErrorEvent>(err);
  }
}

void StreamThread::DispatchEvent(std::shared_ptr<StreamEvent> e) {
  if (event_cb_ != nullptr) {
    event_cb_(e);
  }
}
