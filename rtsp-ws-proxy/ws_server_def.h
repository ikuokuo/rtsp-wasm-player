#pragma once

#include <functional>
#include <string>

#include <boost/asio/spawn.hpp>

#include "common/net/asio.hpp"
#include "common/net/beast.hpp"
#include "common/net/cors.h"

#include "ws_def.h"

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

  net::CorsOptions cors{};

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

  on_fail_t on_fail = nullptr;
  on_stop_t on_stop = nullptr;
  on_exit_t on_exit = nullptr;
};

namespace net {

// This is the C++11 equivalent of a generic lambda.
// The function object is used to send an HTTP message.
struct send_lambda {
  net::tcp_stream_t &stream_;
  bool &close_;
  beast::error_code &ec_;
  asio::yield_context yield_;

  send_lambda(
      net::tcp_stream_t &stream,
      bool &close,
      beast::error_code &ec,
      asio::yield_context yield)
    : stream_(stream),
      close_(close),
      ec_(ec),
      yield_(yield) {
  }

  template <bool isRequest, class Body, class Fields>
  void operator()(http::message<isRequest, Body, Fields> &&msg) const {
    // Determine if we should close the connection after
    close_ = msg.need_eof();

    // We need the serializer here because the serializer requires
    // a non-const file_body, and the message oriented version of
    // http::write only works with const messages.
    http::serializer<isRequest, Body, Fields> sr{msg};
    http::async_write(stream_, sr, yield_[ec_]);
  }
};

}  // namespace net
