#pragma once

#include <string>

#include <boost/beast/core.hpp>
#include <boost/asio/spawn.hpp>

struct WsClientOptions {
  std::string host = "127.0.0.1";
  int port = 8080;

  using on_fail_t =
      std::function<void(boost::beast::error_code ec, char const* what)>;
  on_fail_t on_fail = nullptr;
};

class WsClient {
 public:
  explicit WsClient(const WsClientOptions &options);
  ~WsClient();

  void Run();

 private:
  void OnFail(boost::beast::error_code ec, char const* what);

  void DoSession(
      boost::asio::io_context &ioc,  // NOLINT
      boost::asio::yield_context yield);

  WsClientOptions options_;
};
