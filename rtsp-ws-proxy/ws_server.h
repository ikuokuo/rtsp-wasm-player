#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/spawn.hpp>

#include "common/net/cors.h"
#include "ws_ext.h"

struct WsServerOptions {
  std::string addr  = "0.0.0.0";
  int port          = 8080;
  int threads       = 3;

  bool http_enable          = true;
  // Notice
  //  "": deny access the filesystem
  //  "/", "//": will access the filesystem root
  std::string http_doc_root = ".";

  net::Options cors{};

  bool signal_exit_enable = true;
  bool thread_main_block = true;
  bool thread_exit_block = true;

  using on_fail_t =
      std::function<void(boost::beast::error_code ec, char const *what)>;
  using on_stop_t = std::function<void()>;
  using on_exit_t = std::function<void()>;

  on_fail_t on_fail = ws_ext::fail;
  on_stop_t on_stop = nullptr;
  on_exit_t on_exit = nullptr;
};

class WsServer {
 public:
  using http_req_t = boost::beast::http::request<
      boost::beast::http::string_body,
      boost::beast::http::basic_fields<std::allocator<char>>>;

  explicit WsServer(const WsServerOptions &options);
  virtual ~WsServer();

  void Run();

 protected:
  virtual void OnFail(boost::beast::error_code ec, char const *what);

  virtual bool OnHandleHttpRequest(
      http_req_t &req,
      ws_ext::send_lambda &send);

  virtual bool OnHandleWebSocket(
      boost::beast::websocket::stream<boost::beast::tcp_stream> &ws,
      boost::optional<http_req_t> &req,
      boost::beast::error_code &ec,
      boost::asio::yield_context yield);

  WsServerOptions options_;

 private:
  void DoListen(
      boost::asio::io_context &ioc,
      boost::asio::ip::tcp::endpoint endpoint,
      boost::asio::yield_context yield);

  void DoSessionHTTP(
      boost::beast::tcp_stream &stream,
      boost::asio::yield_context yield);

  void DoSessionWebSocket(
      boost::beast::websocket::stream<boost::beast::tcp_stream> &ws,
      boost::optional<http_req_t> &req,
      boost::asio::yield_context yield);
};
