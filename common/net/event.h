#pragma once

#include <memory>
#include <string>
#include <utility>

#include "common/net/asio.hpp"
#include "common/net/beast.hpp"
#include "common/util/event.h"

namespace net {

enum NetEventId {
  NET_EVENT_FAIL,
  NET_EVENT_OPEN,
  NET_EVENT_OPENED,
  NET_EVENT_SEND,
  NET_EVENT_RECV,
  NET_EVENT_CLOSE,
  NET_EVENT_CLOSED,
};

using NetEvent = Event<NetEventId>;

class NetFailEvent : public NetEvent {
 public:
  NetFailEvent(beast::error_code ec, char const *what)
    : NetEvent{NET_EVENT_FAIL}, ec{ec}, what{what} {
  }
  virtual ~NetFailEvent() = default;
  beast::error_code ec;
  std::string what;
};

class NetSendEvent : public NetEvent {
 public:
  explicit NetSendEvent(std::shared_ptr<void> data)
    : NetEvent{NET_EVENT_SEND}, data{std::move(data)} {
  }
  virtual ~NetSendEvent() = default;
  std::shared_ptr<void> data;
};

class NetRecvEvent : public NetEvent {
 public:
  NetRecvEvent(beast::flat_buffer &buffer, std::size_t bytes_n)
    : NetEvent{NET_EVENT_RECV}, buffer{buffer}, bytes_n{bytes_n} {
  }
  virtual ~NetRecvEvent() = default;
  beast::flat_buffer &buffer;
  std::size_t bytes_n;
};

class NetEventManager : public EventManager<NetEventId> {
 public:
  NetEventManager() = default;
  virtual ~NetEventManager() = default;

 protected:
  virtual void OnEventFail(beast::error_code ec, char const *what) {
    DispatchEvent<NetFailEvent>(ec, what);
  }

  virtual void OnEventOpen() {
    DispatchEvent<NetEvent>(NET_EVENT_OPEN);
  }

  virtual void OnEventOpened() {
    DispatchEvent<NetEvent>(NET_EVENT_OPENED);
  }

  virtual void OnEventSend(std::shared_ptr<void> data) {
    DispatchEvent<NetSendEvent>(std::move(data));
  }

  virtual void OnEventRecv(beast::flat_buffer &buffer, std::size_t bytes_n) {
    DispatchEvent<NetRecvEvent>(buffer, bytes_n);
  }

  virtual void OnEventClose() {
    DispatchEvent<NetEvent>(NET_EVENT_CLOSE);
  }

  virtual void OnEventClosed() {
    DispatchEvent<NetEvent>(NET_EVENT_CLOSED);
  }
};

}  // namespace net
