#pragma once

#include <functional>
#include <memory>
#include <string>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/spawn.hpp>

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
  explicit WsServer(const WsServerOptions &options);
  ~WsServer();

  void Run();

 private:
  void OnFail(boost::beast::error_code ec, char const *what);

  void DoListen(
      boost::asio::io_context &ioc,
      boost::asio::ip::tcp::endpoint endpoint,
      boost::asio::yield_context yield);

  void DoSessionHTTP(
      boost::beast::tcp_stream &stream,
      boost::asio::yield_context yield);

  void DoSessionWebSocket(
      boost::beast::websocket::stream<boost::beast::tcp_stream> &ws,
      boost::asio::yield_context yield);

  template <class Body, class Allocator>
  void DoSessionWebSocketX(
      boost::beast::websocket::stream<boost::beast::tcp_stream> &ws,
      boost::optional<boost::beast::http::request<
          Body, boost::beast::http::basic_fields<Allocator>>> req,
      boost::asio::yield_context yield);

  WsServerOptions options_;
};

template <class Body, class Allocator>
void WsServer::DoSessionWebSocketX(
    boost::beast::websocket::stream<boost::beast::tcp_stream> &ws,
    boost::optional<boost::beast::http::request<
        Body, boost::beast::http::basic_fields<Allocator>>> req,
    boost::asio::yield_context yield) {
  namespace beast = boost::beast;
  namespace http = boost::beast::http;
  namespace websocket = boost::beast::websocket;

  beast::error_code ec;

  // Set suggested timeout settings for the websocket
  ws.set_option(
      websocket::stream_base::timeout::suggested(beast::role_type::server));

  // Set a decorator to change the Server of the handshake
  ws.set_option(websocket::stream_base::decorator(
      [](websocket::response_type &res) {
        res.set(http::field::server,
                std::string(BOOST_BEAST_VERSION_STRING) + " ws-server");
      }));

  // Accept the websocket handshake
  if (req.has_value()) {
    ws.async_accept(req.get(), yield[ec]);
  } else {
    ws.async_accept(yield[ec]);
  }
  if (ec) return OnFail(ec, "accept");

  for (;;) {
    // This buffer will hold the incoming message
    beast::flat_buffer buffer;

    // Read a message
    ws.async_read(buffer, yield[ec]);

    // This indicates that the session was closed
    if (ec == websocket::error::closed) break;

    if (ec) return OnFail(ec, "read");

    // Echo the message back
    ws.text(ws.got_text());
    ws.async_write(buffer.data(), yield[ec]);
    if (ec) return OnFail(ec, "write");
  }
}
