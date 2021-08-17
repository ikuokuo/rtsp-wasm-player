#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include "stream.h"

enum StreamEventId {
  STREAM_EVENT_OPEN,
  STREAM_EVENT_OPENED,
  STREAM_EVENT_GET_PACKET,
  STREAM_EVENT_GET_FRAME,
  STREAM_EVENT_CLOSE,
  STREAM_EVENT_CLOSED,
  STREAM_EVENT_LOOP,
  STREAM_EVENT_ERROR,
};

class StreamEvent {
 public:
  StreamEvent(StreamEventId id, std::shared_ptr<Stream> stream)
      : id(id), stream(stream) {}
  virtual ~StreamEvent() = default;
  StreamEventId id;
  std::shared_ptr<Stream> stream;
};

class StreamErrorEvent : public StreamEvent {
 public:
  StreamErrorEvent(std::shared_ptr<Stream> stream, const StreamError &error)
      : StreamEvent(StreamEventId::STREAM_EVENT_ERROR, stream), error(error) {}
  virtual ~StreamErrorEvent() = default;
  StreamError error;
};

class StreamPacketEvent : public StreamEvent {
 public:
  StreamPacketEvent(std::shared_ptr<Stream> stream, AVPacket *packet)
    : StreamEvent(StreamEventId::STREAM_EVENT_GET_PACKET, stream),
      packet(packet) {}
  virtual ~StreamPacketEvent() = default;
  AVPacket *packet;
};

class StreamFrameEvent : public StreamEvent {
 public:
  StreamFrameEvent(std::shared_ptr<Stream> stream,
                   AVMediaType type,
                   AVFrame *frame)
    : StreamEvent(StreamEventId::STREAM_EVENT_GET_FRAME, stream),
      type(type),
      frame(frame) {}
  virtual ~StreamFrameEvent() = default;
  AVMediaType type;
  AVFrame *frame;
};

class StreamThread : public std::enable_shared_from_this<StreamThread> {
 public:
  using event_callback_t =
      std::function<void(const std::shared_ptr<StreamEvent> &e)>;
  using running_callback_t =
      std::function<void(const std::shared_ptr<StreamThread> &t,
                         const std::shared_ptr<Stream> &s)>;

  StreamThread(std::initializer_list<AVMediaType> types = {AVMEDIA_TYPE_VIDEO},
               bool loop_on_eof = true);
  ~StreamThread();

  bool IsRunning() const;

  void SetEventCallback(event_callback_t cb);
  void SetRunningCallback(running_callback_t cb);

  void Start(const StreamOptions &options, int frequency = 20);
  void Stop();

  void DispatchEvent(std::shared_ptr<StreamEvent> e);

  template <typename E, typename... Args>
  void DispatchEvent(Args&&... args) {
    DispatchEvent(std::make_shared<E>(std::forward<Args>(args)...));
  }

 private:
  void Run();

  std::vector<AVMediaType> get_types_;
  bool loop_on_eof_;

  StreamOptions options_;
  int frequency_;
  event_callback_t event_cb_;
  running_callback_t running_cb_;

  std::atomic_bool is_running_;
  std::thread thread_;
};
