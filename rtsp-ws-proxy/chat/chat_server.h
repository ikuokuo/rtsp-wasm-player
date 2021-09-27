#pragma once

#include <memory>

#include "../ws_server.h"

class ChatRoom;

class ChatServer : public WsServer {
 public:
  explicit ChatServer(const WsServerOptions &options);
  ~ChatServer() override;

 protected:
  void DoSessionWebSocket(
      websocket::stream<beast::tcp_stream> &&ws,
      boost::optional<http_req_t> &&req) override;

  std::shared_ptr<ChatRoom> room_;
};
