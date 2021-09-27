#pragma once

#include <memory>

#include "common/net/ext.h"

#include "ws_server_def.h"

class WsServer {
 public:
  using http_req_t = beast::http::request<
      beast::http::string_body,
      beast::http::basic_fields<std::allocator<char>>>;

  explicit WsServer(const WsServerOptions &options);
  virtual ~WsServer();

  void Run();

 protected:
  virtual void OnFail(beast::error_code ec, char const *what);

  void DoListen(
      asio::io_context &ioc,
      asio::ip::tcp::endpoint endpoint,
      asio::yield_context yield);

  void DoSessionHTTP(
      beast::tcp_stream &stream,
      asio::yield_context yield);

  virtual void DoSessionWebSocket(
      beast::websocket::stream<beast::tcp_stream> &&ws,
      boost::optional<http_req_t> &&req);

  virtual bool OnHandleHttpRequest(
      http_req_t &req,
      net::send_lambda &send);

  WsServerOptions options_;
};
