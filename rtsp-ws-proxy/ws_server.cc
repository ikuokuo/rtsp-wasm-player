#include "ws_server.h"

#include <glog/logging.h>

#include <functional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;

WsServer::WsServer(const std::string &addr, int port, int threads)
  : WsServer(
      asio::ip::tcp::endpoint{
        asio::ip::make_address(addr), static_cast<asio::ip::port_type>(port)},
      threads) {
}

WsServer::WsServer(const boost::asio::ip::tcp::endpoint &endpoint, int threads)
  : endpoint_(endpoint), threads_(threads) {
}

WsServer::~WsServer() {
}

void WsServer::Run() {
  // The io_context is required for all I/O
  asio::io_context ioc(threads_);

  // Spawn a listening port
  asio::spawn(ioc, std::bind(
      &WsServer::DoListen,
      this,
      std::ref(ioc),
      endpoint_,
      std::placeholders::_1));

  // Run the I/O service on the requested number of threads
  std::vector<std::thread> v;
  v.reserve(threads_ - 1);
  for (auto i = threads_ - 1; i > 0; --i) {
    v.emplace_back([&ioc] {
      ioc.run();
    });
  }
  ioc.run();
}

// Accepts incoming connections and launches the sessions
void WsServer::DoListen(
    asio::io_context &ioc,  // NOLINT
    asio::ip::tcp::endpoint endpoint,
    asio::yield_context yield) {
  beast::error_code ec;

  // Open the acceptor
  asio::ip::tcp::acceptor acceptor(ioc);
  acceptor.open(endpoint.protocol(), ec);
  if (ec) return OnError(ec, "open");

  // Allow address reuse
  acceptor.set_option(asio::socket_base::reuse_address(true), ec);
  if (ec) return OnError(ec, "set_option");

  // Bind to the server address
  acceptor.bind(endpoint, ec);
  if (ec) return OnError(ec, "bind");

  // Start listening for connections
  acceptor.listen(asio::socket_base::max_listen_connections, ec);
  if (ec) return OnError(ec, "listen");

  for (;;) {
    asio::ip::tcp::socket socket(ioc);
    acceptor.async_accept(socket, yield[ec]);
    if (ec) {
      OnError(ec, "accept");
    } else {
      asio::spawn(acceptor.get_executor(), std::bind(
          &WsServer::DoSession,
          this,
          websocket::stream<beast::tcp_stream>(std::move(socket)),
          std::placeholders::_1));
    }
  }
}

// Echoes back all received WebSocket messages
void WsServer::DoSession(
    beast::websocket::stream<beast::tcp_stream> &ws,  // NOLINT
    asio::yield_context yield) {
  beast::error_code ec;

  // Set suggested timeout settings for the websocket
  ws.set_option(
      websocket::stream_base::timeout::suggested(beast::role_type::server));

  // Set a decorator to change the Server of the handshake
  ws.set_option(websocket::stream_base::decorator(
      [](websocket::response_type& res) {
        res.set(http::field::server,
                std::string(BOOST_BEAST_VERSION_STRING) + " ws-server");
      }));

  // Accept the websocket handshake
  ws.async_accept(yield[ec]);
  if (ec) return OnError(ec, "accept");

  for (;;) {
    // This buffer will hold the incoming message
    beast::flat_buffer buffer;

    // Read a message
    ws.async_read(buffer, yield[ec]);

    // This indicates that the session was closed
    if (ec == websocket::error::closed) break;

    if (ec) return OnError(ec, "read");

    // Echo the message back
    ws.text(ws.got_text());
    ws.async_write(buffer.data(), yield[ec]);
    if (ec) return OnError(ec, "write");
  }
}

void WsServer::OnError(beast::error_code ec, char const *what) {
  LOG(ERROR) << what << ": " << ec.message();
}
