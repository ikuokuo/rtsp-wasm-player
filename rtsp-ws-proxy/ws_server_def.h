#pragma once

#include <functional>
#include <string>

#include "common/net/asio.hpp"
#include "common/net/beast.hpp"
#include "common/net/cors.h"

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
