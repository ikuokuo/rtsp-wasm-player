#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "common/net/asio.hpp"
#include "common/net/beast.hpp"
#include "common/net/event.h"
#include "common/util/log.h"

struct WsClientOptions {
  std::string host = "127.0.0.1";
  int port = 8080;
  std::string target = "/";
};

template <typename Data>
class WsClient
  : public net::NetEventManager,
    public std::enable_shared_from_this<WsClient<Data>> {
 public:
  using ws_stream_t = websocket::stream<beast::tcp_stream>;
  using std::enable_shared_from_this<WsClient<Data>>::shared_from_this;

  WsClient(asio::io_context &ioc, const WsClientOptions &options);
  virtual ~WsClient();

  void Open();
  void Send(const std::shared_ptr<Data> &data);
  void Close();

 protected:
  void DoResolve();
  void OnResolve(
      beast::error_code ec, tcp::resolver::results_type results);
  void OnConnect(
      beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);
  void OnHandshake(beast::error_code ec);

  void DoRead();
  void OnRead(beast::error_code ec, std::size_t bytes_transferred);
  void DoSend(const std::shared_ptr<Data> &data);
  void DoWrite(const std::shared_ptr<Data> &data);
  void OnWrite(beast::error_code ec, std::size_t bytes_transferred);
  void DoClose();
  void OnClose(beast::error_code ec);

  asio::io_context &ioc_;
  tcp::resolver resolver_;
  websocket::stream<beast::tcp_stream> ws_;
  WsClientOptions options_;

  beast::flat_buffer read_buffer_;
  std::vector<std::shared_ptr<Data>> send_queue_;
};

template <typename Data>
WsClient<Data>::WsClient(asio::io_context &ioc, const WsClientOptions &options)
  : ioc_(ioc),
    resolver_(asio::make_strand(ioc)),
    ws_(asio::make_strand(ioc)),
    options_(options) {
  VLOG(2) << __func__;
}

template <typename Data>
WsClient<Data>::~WsClient() {
  VLOG(2) << __func__;
}

template <typename Data>
void WsClient<Data>::Open() {
  asio::post(
      ws_.get_executor(),
      beast::bind_front_handler(
          &WsClient::DoResolve,
          shared_from_this()));
}

template <typename Data>
void WsClient<Data>::Send(const std::shared_ptr<Data> &data) {
  asio::post(
      ws_.get_executor(),
      beast::bind_front_handler(
          &WsClient::DoSend,
          shared_from_this(),
          data));
}

template <typename Data>
void WsClient<Data>::Close() {
  asio::post(
      ws_.get_executor(),
      beast::bind_front_handler(
          &WsClient::DoClose,
          shared_from_this()));
}

template <typename Data>
void WsClient<Data>::DoResolve() {
  OnEventOpen();
  auto host = options_.host;
  auto port = std::to_string(options_.port);
  // Look up the domain name
  resolver_.async_resolve(
      host,
      port,
      beast::bind_front_handler(
          &WsClient::OnResolve,
          shared_from_this()));
}

template <typename Data>
void WsClient<Data>::OnResolve(
    beast::error_code ec, tcp::resolver::results_type results) {
  if (ec)
    return OnEventFail(ec, "resolve");

  // Set the timeout for the operation
  beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

  // Make the connection on the IP address we get from a lookup
  beast::get_lowest_layer(ws_).async_connect(
      results,
      beast::bind_front_handler(
          &WsClient::OnConnect,
          shared_from_this()));
}

template <typename Data>
void WsClient<Data>::OnConnect(
    beast::error_code ec, tcp::resolver::results_type::endpoint_type ep) {
  if (ec)
    return OnEventFail(ec, "connect");

  // Turn off the timeout on the tcp_stream, because
  // the websocket stream has its own timeout system.
  beast::get_lowest_layer(ws_).expires_never();

  // Set suggested timeout settings for the websocket
  ws_.set_option(
      websocket::stream_base::timeout::suggested(
          beast::role_type::client));

  // Set a decorator to change the User-Agent of the handshake
  ws_.set_option(websocket::stream_base::decorator(
      [](websocket::request_type& req) {
          req.set(http::field::user_agent,
              std::string(BOOST_BEAST_VERSION_STRING) + " ws-client");
      }));

  // Update the host_ string. This will provide the value of the
  // Host HTTP header during the WebSocket handshake.
  // See https://tools.ietf.org/html/rfc7230#section-5.4
  auto host = options_.host + ':' + std::to_string(ep.port());

  // Perform the websocket handshake
  ws_.async_handshake(host, options_.target,
      beast::bind_front_handler(
          &WsClient::OnHandshake,
          shared_from_this()));
}

template <typename Data>
void WsClient<Data>::OnHandshake(beast::error_code ec) {
  if (ec)
    return OnEventFail(ec, "handshake");
  OnEventOpened();
  DoRead();
}

template <typename Data>
void WsClient<Data>::DoRead() {
  // Read a message into our buffer
  ws_.async_read(
      read_buffer_,
      beast::bind_front_handler(
          &WsClient::OnRead,
          shared_from_this()));
}

template <typename Data>
void WsClient<Data>::OnRead(
    beast::error_code ec, std::size_t bytes_transferred) {
  if (ec)
    return OnEventFail(ec, "read");
  OnEventRecv(read_buffer_, bytes_transferred);
  DoRead();
}

template <typename Data>
void WsClient<Data>::DoSend(const std::shared_ptr<Data> &data) {
  // Always add to queue
  send_queue_.push_back(data);

  // Are we already writing?
  if (send_queue_.size() > 1)
    return;

  // We are not currently writing, so send this immediately
  DoWrite(send_queue_.front());
}

template <typename Data>
void WsClient<Data>::DoWrite(const std::shared_ptr<Data> &data) {
  OnEventSend(data);
  ws_.async_write(
      asio::buffer(*data),
      beast::bind_front_handler(
          &WsClient::OnWrite,
          shared_from_this()));
}

template <typename Data>
void WsClient<Data>::OnWrite(
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

template <typename Data>
void WsClient<Data>::DoClose() {
  OnEventClose();
  // Close the WebSocket connection
  ws_.async_close(websocket::close_code::normal,
      beast::bind_front_handler(
          &WsClient::OnClose,
          shared_from_this()));
}

template <typename Data>
void WsClient<Data>::OnClose(beast::error_code ec) {
  if (ec)
    return OnEventFail(ec, "close");
  // If we get here then the connection is closed gracefully
  OnEventClosed();
}
