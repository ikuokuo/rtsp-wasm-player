#include "ws_server.h"

#include <glog/logging.h>

#include <thread>
#include <utility>
#include <vector>

#include <boost/asio/signal_set.hpp>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = boost::asio::ip::tcp;

WsServer::WsServer(const WsServerOptions &options)
  : options_(options) {
}

WsServer::~WsServer() {
}

void WsServer::Run() {
  auto endpoint = tcp::endpoint{
      asio::ip::make_address(options_.addr),
      static_cast<asio::ip::port_type>(options_.port)};

  // ws_ext::run_server(endpoint, options_.threads, options_.doc_root);
  // return;

  // The io_context is required for all I/O
  asio::io_context ioc(options_.threads);

  // Spawn a listening port
  asio::spawn(ioc, std::bind(
      &WsServer::DoListen,
      this,
      std::ref(ioc),
      endpoint,
      std::placeholders::_1));

  // Capture SIGINT and SIGTERM to perform a clean shutdown
  std::unique_ptr<asio::signal_set> signals{nullptr};
  if (options_.signal_exit_enable) {
    signals.reset(new asio::signal_set(ioc, SIGINT, SIGTERM));
    signals->async_wait([&](beast::error_code const&, int) {
      // Stop the `io_context`. This will cause `run()`
      // to return immediately, eventually destroying the
      // `io_context` and all of the sockets in it.
      ioc.stop();
      if (options_.on_stop) options_.on_stop();
    });
  }

  // Run the I/O service on the requested number of threads
  std::vector<std::thread> v;
  if (options_.thread_main_block) {
    v.reserve(options_.threads - 1);
    for (auto i = options_.threads - 1; i > 0; --i) {
      v.emplace_back([&ioc] {
        ioc.run();
      });
    }
    ioc.run();
    // (If we get here, it means we got a SIGINT or SIGTERM)
  } else {
    v.reserve(options_.threads);
    for (auto i = options_.threads; i > 0; --i) {
      v.emplace_back([&ioc] {
        ioc.run();
      });
    }
  }

  if (options_.on_exit) options_.on_exit();

  if (options_.thread_exit_block) {
    // Block until all the threads exit
    for (auto &t : v)
      t.join();
  }
}

void WsServer::OnFail(beast::error_code ec, char const *what) {
  if (options_.on_fail) {
    options_.on_fail(ec, what);
  }
}

bool WsServer::OnHandleHttpRequest(
    http_req_t &req,
    ws_ext::send_lambda &send) {
  (void)req;
  (void)send;
  return false;
}

bool WsServer::OnHandleWebSocket(
    beast::websocket::stream<beast::tcp_stream> &ws,
    boost::optional<http_req_t> &req,
    beast::error_code &ec,
    asio::yield_context yield) {
  (void)req;
  /*
  beast::string_view target;
  if (req.has_value()) {
    target = req.get().target();
  }
  */
  // Echoes back all received WebSocket messages
  for (;;) {
    // This buffer will hold the incoming message
    beast::flat_buffer buffer;

    // Read a message
    ws.async_read(buffer, yield[ec]);

    // This indicates that the session was closed
    if (ec == websocket::error::closed) break;

    if (ec) {
      OnFail(ec, "read");
      break;
    }

    // Echo the message back
    ws.text(ws.got_text());
    ws.async_write(buffer.data(), yield[ec]);
    if (ec) {
      OnFail(ec, "write");
      break;
    }
  }

  return true;
}

// Accepts incoming connections and launches the sessions
void WsServer::DoListen(
    asio::io_context &ioc,
    tcp::endpoint endpoint,
    asio::yield_context yield) {
  beast::error_code ec;

  // Open the acceptor
  tcp::acceptor acceptor(ioc);
  acceptor.open(endpoint.protocol(), ec);
  if (ec) return OnFail(ec, "open");

  // Allow address reuse
  acceptor.set_option(asio::socket_base::reuse_address(true), ec);
  if (ec) return OnFail(ec, "set_option");

  // Bind to the server address
  acceptor.bind(endpoint, ec);
  if (ec) return OnFail(ec, "bind");

  // Start listening for connections
  acceptor.listen(asio::socket_base::max_listen_connections, ec);
  if (ec) return OnFail(ec, "listen");

  for (;;) {
    tcp::socket socket(ioc);
    acceptor.async_accept(socket, yield[ec]);
    if (ec) {
      OnFail(ec, "accept");
    } else {
      if (options_.http_enable) {
        asio::spawn(acceptor.get_executor(), std::bind(
            &WsServer::DoSessionHTTP,
            this,
            beast::tcp_stream(std::move(socket)),
            std::placeholders::_1));
      } else {
        asio::spawn(acceptor.get_executor(), std::bind(
            &WsServer::DoSessionWebSocket,
            this,
            websocket::stream<beast::tcp_stream>(std::move(socket)),
            boost::optional<http_req_t>(boost::none),
            std::placeholders::_1));
      }
    }
  }
}

// Handles an HTTP server connection
void WsServer::DoSessionHTTP(
    beast::tcp_stream &stream,
    asio::yield_context yield) {
  bool close = false;
  beast::error_code ec;

  // This buffer is required to persist across reads
  beast::flat_buffer buffer;

  // The parser is stored in an optional container so we can
  // construct it from scratch it at the beginning of each new message.
  boost::optional<http::request_parser<http::string_body>> parser;

  // This lambda is used to send messages
  ws_ext::send_lambda lambda{stream, close, ec, yield};

  for (;;) {
    // Construct a new parser for each message
    parser.emplace();

    // Apply a reasonable limit to the allowed size
    // of the body in bytes to prevent abuse.
    parser->body_limit(10000);

    // Set the timeout.
    stream.expires_after(std::chrono::seconds(30));

    // Read a request
    http::async_read(stream, buffer, *parser, yield[ec]);
    if (ec == http::error::end_of_stream)
      break;
    if (ec)
      return OnFail(ec, "read");

    auto http_req = parser->release();

    // See if it is a WebSocket Upgrade
    if (websocket::is_upgrade(http_req)) {
      // LOG(INFO) << "ws req: " << http_req.target();
      auto ws = websocket::stream<beast::tcp_stream>(
          std::move(stream.release_socket()));
      auto req = boost::optional<http_req_t>(http_req);
      DoSessionWebSocket(ws, req, yield);
      return;
    }

    // LOG(INFO) << "http req: " << http_req.target();
    auto handled = OnHandleHttpRequest(http_req, lambda);
    if (!handled) {
      ws_ext::handle_request(options_.http_doc_root,
                             std::move(http_req), lambda);
      if (ec) return OnFail(ec, "write");
    }

    if (close) {
      // This means we should close the connection, usually because
      // the response indicated the "Connection: close" semantic.
      break;
    }
  }

  // Send a TCP shutdown
  stream.socket().shutdown(tcp::socket::shutdown_send, ec);

  // At this point the connection is closed gracefully
}

// Handles an WebSocket server connection
void WsServer::DoSessionWebSocket(
    boost::beast::websocket::stream<boost::beast::tcp_stream> &ws,
    boost::optional<http_req_t> &req,
    boost::asio::yield_context yield) {
  beast::error_code ec;

  // Set suggested timeout settings for the websocket
  ws.set_option(
      websocket::stream_base::timeout::suggested(beast::role_type::server));

  // Set a decorator to change the Server of the handshake
  ws.set_option(websocket::stream_base::decorator(
      [](websocket::response_type &res) {
        res.set(http::field::server,
                std::string(BOOST_BEAST_VERSION_STRING) + " ws-server");
      }));

  // Accept the websocket handshake
  if (req.has_value()) {
    ws.async_accept(req.get(), yield[ec]);
  } else {
    ws.async_accept(yield[ec]);
  }
  if (ec) return OnFail(ec, "accept");

  OnHandleWebSocket(ws, req, ec, yield);
}
