#pragma once

#ifndef MY_USE_SSL
# define MY_USE_SSL  // correct lint
#endif
#include "ws_server_def.h"

class WsServerSSL {
 public:
  using tcp_stream_t = net::tcp_stream_t;
  using ws_stream_t = net::ws_stream_t;
  using http_req_t = net::http_req_t;
  using send_lambda_t = net::send_lambda;

  explicit WsServerSSL(const WsServerOptions &options);
  virtual ~WsServerSSL();

  void Run();

 protected:
  void LoadServerCertificate(ssl::context &ctx);

  virtual void OnFail(beast::error_code ec, char const *what);

  void DoListen(
      asio::io_context &ioc,
      ssl::context &ctx,
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
