#pragma once

#include <string>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/spawn.hpp>

class WsClient {
 public:
  WsClient(const std::string &host, int port);
  ~WsClient();

  void Run();

 private:
  void DoSession(
      boost::asio::io_context &ioc,  // NOLINT
      boost::asio::yield_context yield);

  void OnError(boost::beast::error_code ec, char const *what);

  std::string host_;
  int port_;
};
