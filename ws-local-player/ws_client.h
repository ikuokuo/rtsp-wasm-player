#pragma once

#include <string>

#include <boost/asio/spawn.hpp>

#include "common/net/asio.hpp"
#include "common/net/beast.hpp"

struct WsClientOptions {
  std::string host = "127.0.0.1";
  int port = 8080;

  std::string target = "/";

  using on_fail_t =
      std::function<void(beast::error_code ec, char const *what)>;
  on_fail_t on_fail = nullptr;
};

class WsClient {
 public:
  explicit WsClient(const WsClientOptions &options);
  virtual ~WsClient();

  virtual void Run();

 protected:
  virtual void OnFail(beast::error_code ec, char const *what);

  virtual bool OnRead(beast::flat_buffer *buffer);

  void DoSession(
      asio::io_context &ioc,
      asio::yield_context yield);

  WsClientOptions options_;
};
