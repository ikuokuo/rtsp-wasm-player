#include "ws_stream_session.h"

#include <utility>

#include <boost/lexical_cast.hpp>

#include "common/util/log.h"

#include "ws_stream_room.h"

WsStreamSession::WsStreamSession(
    ws_stream_t &&ws,
    boost::optional<http_req_t> &&req,
    std::size_t send_queue_max_size,
    std::string id,
    std::shared_ptr<WsStreamRoom> room)
  : WsSession(std::move(ws), std::move(req),
      id + "|" + boost::lexical_cast<std::string>(
          beast::get_lowest_layer(ws).socket().remote_endpoint()),
      send_queue_max_size),
    id_(std::move(id)), room_(std::move(room)) {
  VLOG(2) << __func__ << "[" << tag_ << "]";
}

WsStreamSession::~WsStreamSession() {
  VLOG(2) << __func__ << "[" << tag_ << "]";
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
    VLOG(2) << "WsStreamSession[" << tag_ << "] send bytes_n=" << d->size();
  }
}
