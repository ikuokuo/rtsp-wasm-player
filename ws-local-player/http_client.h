#pragma once

#include <functional>
#include <string>

#include <boost/asio/spawn.hpp>

#include "common/net/asio.hpp"
#include "common/net/beast.hpp"

struct HttpClientOptions {
  std::string host = "127.0.0.1";
  int port = 8080;

  std::string target = "/";
  int timeout = 10;  // secs
  int version = 11;  // HTTP version: 1.0 or 1.1(default)

  using on_fail_t =
      std::function<void(beast::error_code ec, char const *what)>;
  using response_t = http::response<http::dynamic_body>;
  using on_response_t = std::function<void(const response_t &)>;

  on_fail_t on_fail = nullptr;
  on_response_t on_response = nullptr;
};

class HttpClient {
 public:
  using response_t = HttpClientOptions::response_t;
  using on_response_t = HttpClientOptions::on_response_t;

  explicit HttpClient(const HttpClientOptions &options);
  virtual ~HttpClient();

  void Run(on_response_t cb = nullptr);

 protected:
  virtual void OnFail(beast::error_code ec, char const *what);

  virtual void OnResponse(const response_t &);

  void NotifyResponse(const response_t &);

 private:
  void DoSession(asio::io_context &ioc, asio::yield_context yield);

  HttpClientOptions options_;
  on_response_t run_response_cb_;
};
