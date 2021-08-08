#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <utility>

#include "stream.h"

enum StreamEventId {
  STREAM_EVENT_OPEN,
  STREAM_EVENT_OPENED,
  STREAM_EVENT_GET_PACKET,
  STREAM_EVENT_GET_FRAME,
  STREAM_EVENT_CLOSE,
  STREAM_EVENT_CLOSED,
  STREAM_EVENT_ERROR,
};

class StreamEvent {
 public:
  explicit StreamEvent(StreamEventId id) : id(id) {}
  virtual ~StreamEvent() = default;
  StreamEventId id;
};

class StreamErrorEvent : public StreamEvent {
 public:
  explicit StreamErrorEvent(const StreamError &error)
    : StreamEvent(StreamEventId::STREAM_EVENT_ERROR), error(error) {}
  virtual ~StreamErrorEvent() = default;
  StreamError error;
};

class StreamPacketEvent : public StreamEvent {
 public:
  explicit StreamPacketEvent(AVPacket *packet)
    : StreamEvent(StreamEventId::STREAM_EVENT_GET_PACKET), packet(packet) {}
  virtual ~StreamPacketEvent() = default;
  AVPacket *packet;
};

class StreamFrameEvent : public StreamEvent {
 public:
  StreamFrameEvent(AVMediaType type, AVFrame *frame)
    : StreamEvent(StreamEventId::STREAM_EVENT_GET_FRAME), type(type),
      frame(frame) {}
  virtual ~StreamFrameEvent() = default;
  AVMediaType type;
  AVFrame *frame;
};

class StreamThread {
 public:
  using event_callback_t = std::function<void(std::shared_ptr<StreamEvent> e)>;

  StreamThread();
  ~StreamThread();

  bool IsRunning() const;
  void Start(const StreamOptions &options, int frequency = 20);
  void SetEventCallback(event_callback_t cb);
  void Stop();

 private:
  void Run();

  void DispatchEvent(std::shared_ptr<StreamEvent> e);

  template <typename E, typename... Args>
  void DispatchEvent(Args&&... args) {
    DispatchEvent(std::make_shared<E>(std::forward<Args>(args)...));
  }

  StreamOptions options_;
  int frequency_;
  event_callback_t event_cb_;

  std::atomic_bool is_running_;
  std::thread thread_;
};
