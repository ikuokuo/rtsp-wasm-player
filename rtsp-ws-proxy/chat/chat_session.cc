#include "chat_session.h"

#include <utility>

#include <boost/lexical_cast.hpp>

#include "common/util/log.h"

#include "chat_room.h"

ChatSession::ChatSession(
    ws_stream_t &&ws,
    boost::optional<http_req_t> &&req,
    std::shared_ptr<ChatRoom> room)
  : WsSession(std::move(ws), std::move(req),
        boost::lexical_cast<std::string>(
            ws.next_layer().socket().remote_endpoint()),
        5),
    room_(std::move(room)) {
  VLOG(2) << __func__ << "[" << tag_ << "]";
}

ChatSession::~ChatSession() {
  VLOG(2) << __func__ << "[" << tag_ << "]";
}

void ChatSession::OnEventOpened() {
  room_->Send(std::make_shared<std::string>(tag_ + " Join"));
  room_->Join(shared_from_this());
  WsSession<std::string>::OnEventOpened();
}

void ChatSession::OnEventClosed() {
  WsSession<std::string>::OnEventClosed();
  room_->Leave(shared_from_this());
  room_->Send(std::make_shared<std::string>(tag_ + " Leave"));
}

void ChatSession::OnEventSend(std::shared_ptr<void> data) {
  if (VLOG_IS_ON(2)) {
    auto d = std::static_pointer_cast<data_t>(data);
    VLOG(2) << " Send[" << tag_ << "]: " << *d;
  }
  // WsSession<std::string>::OnEventSend(data);
}

void ChatSession::OnEventRecv(beast::flat_buffer &buffer, std::size_t bytes_n) {
  // Forward the received messages to the chat room
  // WsSession<std::string>::OnEventRecv(buffer, bytes_n);
  auto s = beast::buffers_to_string(buffer.data());
  VLOG(2) << " Recv[" << tag_ << "]: " << s;
  room_->Send(std::make_shared<std::string>(tag_ + " Say: " + s));
  buffer.consume(bytes_n);
}
