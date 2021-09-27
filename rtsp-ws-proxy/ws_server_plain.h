#pragma once

#include "ws_server_def.h"

class WsServer {
 public:
  using tcp_stream_t = net::tcp_stream_t;
  using ws_stream_t = net::ws_stream_t;
  using http_req_t = net::http_req_t;
  using send_lambda_t = net::send_lambda;

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
      tcp_stream_t &stream, asio::yield_context yield);

  virtual void DoSessionWebSocket(
      ws_stream_t &&ws, boost::optional<http_req_t> &&req);

  virtual bool OnHandleHttpRequest(
      http_req_t &req, send_lambda_t &send);

  WsServerOptions options_;
};
