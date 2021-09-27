#pragma once

#include <memory>

#include "common/net/asio.hpp"
#include "common/net/beast.hpp"

#ifdef MY_USE_SSL
#include <boost/beast/ssl.hpp>
namespace ssl = boost::asio::ssl;
#endif

namespace net {

#ifdef MY_USE_SSL
using tcp_stream_t = beast::ssl_stream<beast::tcp_stream>;
#else
using tcp_stream_t = beast::tcp_stream;
#endif

using ws_stream_t = websocket::stream<tcp_stream_t>;

using http_req_t = http::request<
    http::string_body, http::basic_fields<std::allocator<char>>>;

}  // namespace net
