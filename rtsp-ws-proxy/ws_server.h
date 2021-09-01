#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "common/net/asio.hpp"
#include "common/net/beast.hpp"
#include "common/net/cors.h"

#include "ws_ext.h"

struct WsServerOptions {
  std::string addr  = "0.0.0.0";
  int port          = 8080;
  int threads       = 3;

  struct HTTP {
    bool enable          = true;
    // Notice
    //  "": deny access the filesystem
    //  "/", "//": will access the filesystem root
    std::string doc_root = ".";
  } http{};

  net::Options cors{};

  struct Stream {
    std::string http_target = "/streams";
    std::string ws_target_prefix = "/stream/";
  } stream{};

  bool signal_exit_enable = true;
  bool thread_main_block = true;
  bool thread_exit_block = true;

  using on_fail_t =
      std::function<void(beast::error_code ec, char const *what)>;
  using on_stop_t = std::function<void()>;
  using on_exit_t = std::function<void()>;

  on_fail_t on_fail = ws_ext::fail;
  on_stop_t on_stop = nullptr;
  on_exit_t on_exit = nullptr;
};

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

  virtual bool OnHandleHttpRequest(
      http_req_t &req,
      ws_ext::send_lambda &send);

  virtual bool OnHandleWebSocket(
      beast::websocket::stream<beast::tcp_stream> &ws,
      boost::optional<http_req_t> &req,
      beast::error_code &ec,
      asio::yield_context yield);

  WsServerOptions options_;

 private:
  void DoListen(
      asio::io_context &ioc,
      asio::ip::tcp::endpoint endpoint,
      asio::yield_context yield);

  void DoSessionHTTP(
      beast::tcp_stream &stream,
      asio::yield_context yield);

  void DoSessionWebSocket(
      beast::websocket::stream<beast::tcp_stream> &ws,
      boost::optional<http_req_t> &req,
      asio::yield_context yield);
};
