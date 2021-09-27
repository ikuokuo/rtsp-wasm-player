#include "chat_server.h"

#include <string>
#include <utility>

#include "common/util/log.h"

#include "chat_room.h"
#include "chat_session.h"

ChatServer::ChatServer(const WsServerOptions &options)
  : WsServer(options), room_(std::make_shared<ChatRoom>()) {
  VLOG(2) << __func__;
}

ChatServer::~ChatServer() {
  VLOG(2) << __func__;
}

void ChatServer::DoSessionWebSocket(
    ws_stream_t &&ws, boost::optional<http_req_t> &&req) {
  auto s = std::make_shared<ChatSession>(std::move(ws), std::move(req), room_);
  s->SetEventCallback(net::NET_EVENT_FAIL,
      [this](const std::shared_ptr<ChatSession::event_t> &event) {
        auto e = std::dynamic_pointer_cast<net::NetFailEvent>(event);
        OnFail(e->ec, e->what.c_str());
      });
  s->Run();
}
