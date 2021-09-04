#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "common/net/asio.hpp"
#include "common/net/beast.hpp"
#include "common/net/event.h"
#include "common/util/log.h"
#include "common/util/ptr.h"

template <typename Data>
class WsSession
  : public net::NetEventManager,
    public virtual_enable_shared_from_this<WsSession<Data>> {
 public:
  using data_t = Data;
  using ws_stream_t = websocket::stream<beast::tcp_stream>;
  using http_req_t = beast::http::request<
      beast::http::string_body,
      beast::http::basic_fields<std::allocator<char>>>;
  using virtual_enable_shared_from_this<WsSession<Data>>::shared_from_this;

  WsSession(ws_stream_t &&ws, boost::optional<http_req_t> &&req);
  virtual ~WsSession();

  void Run();
  void Send(const std::shared_ptr<Data> &data);

 protected:
  void OnEventFail(beast::error_code ec, char const *what) override;
  void OnAccept(beast::error_code ec);

  void DoRead();
  void OnRead(beast::error_code ec, std::size_t bytes_transferred);
  void DoSend(const std::shared_ptr<Data> &data);
  void DoWrite(const std::shared_ptr<Data> &data);
  void OnWrite(beast::error_code ec, std::size_t bytes_transferred);

  ws_stream_t ws_;
  boost::optional<http_req_t> req_;

  beast::flat_buffer read_buffer_;
  std::vector<std::shared_ptr<Data>> send_queue_;
};

template <typename Data>
WsSession<Data>::WsSession(ws_stream_t &&ws, boost::optional<http_req_t> &&req)
  : ws_(std::move(ws)), req_(std::move(req)) {
  VLOG(2) << __func__;
}

template <typename Data>
WsSession<Data>::~WsSession() {
  VLOG(2) << __func__;
}

template <typename Data>
void WsSession<Data>::Run() {
  // Set suggested timeout settings for the websocket
  ws_.set_option(
      websocket::stream_base::timeout::suggested(beast::role_type::server));

  // Set a decorator to change the Server of the handshake
  ws_.set_option(websocket::stream_base::decorator(
      [](websocket::response_type &res) {
        res.set(http::field::server,
                std::string(BOOST_BEAST_VERSION_STRING) + " ws-server");
      }));

  OnEventOpen();
  // Accept the websocket handshake
  if (req_.has_value()) {
    ws_.async_accept(
        req_.get(),
        beast::bind_front_handler(
            &WsSession<Data>::OnAccept,
            shared_from_this()));
  } else {
    ws_.async_accept(
        beast::bind_front_handler(
            &WsSession<Data>::OnAccept,
            shared_from_this()));
  }
}

template <typename Data>
void WsSession<Data>::Send(const std::shared_ptr<Data> &data) {
  asio::post(
      ws_.get_executor(),
      beast::bind_front_handler(
          &WsSession::DoSend,
          shared_from_this(),
          data));
}

template <typename Data>
void WsSession<Data>::OnEventFail(beast::error_code ec, char const *what) {
  if (ec == asio::error::operation_aborted || ec == websocket::error::closed) {
    OnEventClosed();
    return;
  }
  net::NetEventManager::OnEventFail(ec, what);
  OnEventClosed();
}

template <typename Data>
void WsSession<Data>::OnAccept(beast::error_code ec) {
  if (ec)
    return OnEventFail(ec, "accept");
  OnEventOpened();
  DoRead();
}

template <typename Data>
void WsSession<Data>::DoRead() {
  // Read a message into our buffer
  ws_.async_read(
      read_buffer_,
      beast::bind_front_handler(
          &WsSession::OnRead,
          shared_from_this()));
}

template <typename Data>
void WsSession<Data>::OnRead(
    beast::error_code ec, std::size_t bytes_transferred) {
  if (ec)
    return OnEventFail(ec, "read");
  OnEventRecv(read_buffer_, bytes_transferred);
  DoRead();
}

template <typename Data>
void WsSession<Data>::DoSend(const std::shared_ptr<Data> &data) {
  // Always add to queue
  send_queue_.push_back(data);

  // Are we already writing?
  if (send_queue_.size() > 1)
    return;

  // We are not currently writing, so send this immediately
  DoWrite(send_queue_.front());
}

template <typename Data>
void WsSession<Data>::DoWrite(const std::shared_ptr<Data> &data) {
  OnEventSend(data);
  ws_.binary(true);
  ws_.async_write(
      asio::buffer(*data),
      beast::bind_front_handler(
          &WsSession::OnWrite,
          shared_from_this()));
}

template <typename Data>
void WsSession<Data>::OnWrite(
    beast::error_code ec, std::size_t bytes_transferred) {
  (void)bytes_transferred;
  if (ec)
    return OnEventFail(ec, "write");

  // Remove the string from the queue
  send_queue_.erase(send_queue_.begin());

  // Send the next message if any
  if (!send_queue_.empty())
    DoWrite(send_queue_.front());
}
