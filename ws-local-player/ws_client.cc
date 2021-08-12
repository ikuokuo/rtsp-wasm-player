#include "ws_client.h"

#include <glog/logging.h>

#include <functional>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;

WsClient::WsClient(const std::string &host, int port)
  : host_(host), port_(port) {
}

WsClient::~WsClient() {
}

void WsClient::Run() {
  // The io_context is required for all I/O
  asio::io_context ioc;

  // Launch the asynchronous operation
  boost::asio::spawn(ioc, std::bind(
      &WsClient::DoSession,
      this,
      std::ref(ioc),
      std::placeholders::_1));

  // Run the I/O service. The call will return when
  // the socket is closed.
  ioc.run();
}

// Sends a WebSocket message and prints the response
void WsClient::DoSession(
    boost::asio::io_context &ioc,  // NOLINT
    boost::asio::yield_context yield) {
  beast::error_code ec;

  // These objects perform our I/O
  asio::ip::tcp::resolver resolver(ioc);
  websocket::stream<beast::tcp_stream> ws(ioc);

  auto host = host_;
  auto port = std::to_string(port_);

  // Look up the domain name
  auto const results = resolver.async_resolve(host, port, yield[ec]);
  if (ec) return OnError(ec, "resolve");

  // Set a timeout on the operation
  beast::get_lowest_layer(ws).expires_after(std::chrono::seconds(30));

  // Make the connection on the IP address we get from a lookup
  auto ep = beast::get_lowest_layer(ws).async_connect(results, yield[ec]);
  if (ec) return OnError(ec, "connect");

  // Update the host string. This will provide the value of the
  // Host HTTP header during the WebSocket handshake.
  // See https://tools.ietf.org/html/rfc7230#section-5.4
  host += ':' + std::to_string(ep.port());

  // Turn off the timeout on the tcp_stream, because
  // the websocket stream has its own timeout system.
  beast::get_lowest_layer(ws).expires_never();

  // Set suggested timeout settings for the websocket
  ws.set_option(
      websocket::stream_base::timeout::suggested(
          beast::role_type::client));

  // Set a decorator to change the User-Agent of the handshake
  ws.set_option(websocket::stream_base::decorator(
      [](websocket::request_type& req) {
        req.set(http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) + " ws-client");
      }));

  // Perform the websocket handshake
  ws.async_handshake(host, "/", yield[ec]);
  if (ec) return OnError(ec, "handshake");

  // Send the message
  ws.async_write(asio::buffer(std::string("hello")), yield[ec]);
  if (ec) return OnError(ec, "write");

  // This buffer will hold the incoming message
  beast::flat_buffer buffer;

  // Read a message into our buffer
  ws.async_read(buffer, yield[ec]);
  if (ec) return OnError(ec, "read");

  // Close the WebSocket connection
  ws.async_close(websocket::close_code::normal, yield[ec]);
  if (ec) return OnError(ec, "close");

  // If we get here then the connection is closed gracefully

  // The make_printable() function helps print a ConstBufferSequence
  LOG(INFO) << beast::make_printable(buffer.data());
}

void WsClient::OnError(boost::beast::error_code ec, char const *what) {
  LOG(ERROR) << what << ": " << ec.message();
}
