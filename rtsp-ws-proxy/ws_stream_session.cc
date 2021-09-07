#include "ws_stream_session.h"

#include <utility>

#include <boost/lexical_cast.hpp>

#include "common/util/log.h"

#include "ws_stream_room.h"

WsStreamSession::WsStreamSession(
    ws_stream_t &&ws,
    boost::optional<http_req_t> &&req,
    std::string id,
    std::shared_ptr<WsStreamRoom> room)
  : WsSession(std::move(ws), std::move(req), 1),
    id_(std::move(id)), room_(std::move(room)) {
  auto ep = ws_.next_layer().socket().remote_endpoint();
  who_ = boost::lexical_cast<std::string>(ep);
  VLOG(2) << __func__ << " " << who_;
}

WsStreamSession::~WsStreamSession() {
  VLOG(2) << __func__ << " " << who_;
}

void WsStreamSession::OnEventOpened() {
  room_->Join(id_, shared_from_this());
  WsSession<data_t>::OnEventOpened();
}

void WsStreamSession::OnEventClosed() {
  WsSession<data_t>::OnEventClosed();
  room_->Leave(id_, shared_from_this());
}

void WsStreamSession::OnEventSend(std::shared_ptr<void> data) {
  if (VLOG_IS_ON(2)) {
    auto d = std::static_pointer_cast<data_t>(data);
    VLOG(2) << "Stream[" << id_ << "] send " << who_
        << ", bytes_n=" << d->size();
  }
}
