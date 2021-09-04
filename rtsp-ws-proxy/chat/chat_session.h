#pragma once

#include <memory>
#include <string>

#include "common/util/ptr.h"

#include "../ws_session.h"

class ChatRoom;

class ChatSession
  : public WsSession<std::string>,
    public virtual_enable_shared_from_this<ChatSession> {
 public:
  using virtual_enable_shared_from_this<ChatSession>::shared_from_this;

  ChatSession(ws_stream_t &&ws, boost::optional<http_req_t> &&req,
      std::shared_ptr<ChatRoom> room);
  ~ChatSession() override;

 protected:
  void OnEventOpened() override;
  void OnEventClosed() override;

  void OnEventSend(std::shared_ptr<void> data) override;
  void OnEventRecv(beast::flat_buffer &buffer, std::size_t bytes_n) override;

  std::shared_ptr<ChatRoom> room_;
  std::string who_;
};
