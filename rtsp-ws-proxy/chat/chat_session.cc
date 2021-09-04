#include "chat_session.h"

#include <utility>

#include <boost/lexical_cast.hpp>

#include "common/util/log.h"

#include "chat_room.h"

ChatSession::ChatSession(
    ws_stream_t &&ws,
    boost::optional<http_req_t> &&req,
    std::shared_ptr<ChatRoom> room)
  : WsSession(std::move(ws), std::move(req)), room_(std::move(room)) {
  auto ep = ws_.next_layer().socket().remote_endpoint();
  who_ = "[" + boost::lexical_cast<std::string>(ep) + "]";
  VLOG(2) << __func__ << who_;
}

ChatSession::~ChatSession() {
  VLOG(2) << __func__ << who_;
}

void ChatSession::OnEventOpened() {
  room_->Send(std::make_shared<std::string>(who_ + " Join"));
  room_->Join(shared_from_this());
  WsSession<std::string>::OnEventOpened();
}

void ChatSession::OnEventClosed() {
  WsSession<std::string>::OnEventClosed();
  room_->Leave(shared_from_this());
  room_->Send(std::make_shared<std::string>(who_ + " Leave"));
}

void ChatSession::OnEventSend(std::shared_ptr<void> data) {
  if (VLOG_IS_ON(2)) {
    auto d = std::static_pointer_cast<data_t>(data);
    VLOG(2) << " Send" << who_ << ": " << *d;
  }
  // WsSession<std::string>::OnEventSend(data);
}

void ChatSession::OnEventRecv(beast::flat_buffer &buffer, std::size_t bytes_n) {
  // Forward the received messages to the chat room
  // WsSession<std::string>::OnEventRecv(buffer, bytes_n);
  auto s = beast::buffers_to_string(buffer.data());
  VLOG(2) << " Recv" << who_ << ": " << s;
  room_->Send(std::make_shared<std::string>(who_ + " Say: " + s));
  buffer.consume(bytes_n);
}
