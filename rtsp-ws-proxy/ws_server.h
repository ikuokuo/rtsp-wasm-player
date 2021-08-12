#pragma once

#include <string>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/spawn.hpp>

class WsServer {
 public:
  WsServer(const std::string &addr, int port, int threads);
  WsServer(const boost::asio::ip::tcp::endpoint &endpoint, int threads);
  ~WsServer();

  void Run();

 private:
  void DoListen(
      boost::asio::io_context &ioc,  // NOLINT
      boost::asio::ip::tcp::endpoint endpoint,
      boost::asio::yield_context yield);

  void DoSession(
      boost::beast::websocket::stream<boost::beast::tcp_stream> &ws,  // NOLINT
      boost::asio::yield_context yield);

  void OnError(boost::beast::error_code ec, char const *what);

  boost::asio::ip::tcp::endpoint endpoint_;
  int threads_;
};
