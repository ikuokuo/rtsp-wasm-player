#pragma once

#include <functional>
#include <string>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/spawn.hpp>

struct HttpClientOptions {
  std::string host = "127.0.0.1";
  int port = 8080;

  std::string target = "/";
  int timeout = 10;  // secs
  int version = 11;  // HTTP version: 1.0 or 1.1(default)

  using on_fail_t =
      std::function<void(boost::beast::error_code ec, char const *what)>;
  using response_t =
    boost::beast::http::response<boost::beast::http::dynamic_body>;
  using on_response_t =
      std::function<void(const response_t &)>;

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
  virtual void OnFail(boost::beast::error_code ec, char const *what);

  virtual void OnResponse(const response_t &);

  void NotifyResponse(const response_t &);

 private:
  void DoSession(
      boost::asio::io_context &ioc,  // NOLINT
      boost::asio::yield_context yield);

  HttpClientOptions options_;
  on_response_t run_response_cb_;
};
