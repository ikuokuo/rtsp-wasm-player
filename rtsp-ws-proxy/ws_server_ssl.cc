#include "ws_server_ssl.h"

#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "common/net/ext.h"
#include "common/net/server_certificate.hpp"
#include "common/util/log.h"
#include "ws_session.h"

WsServerSSL::WsServerSSL(const WsServerOptions &options)
  : options_(options) {
  VLOG(2) << __func__;
}

WsServerSSL::~WsServerSSL() {
  VLOG(2) << __func__;
}

void WsServerSSL::Run() {
  auto endpoint = tcp::endpoint{
      asio::ip::make_address(options_.addr),
      static_cast<asio::ip::port_type>(options_.port)};
  // The io_context is required for all I/O
  asio::io_context ioc(options_.threads);

  // The SSL context is required, and holds certificates
  ssl::context ctx{ssl::context::tlsv12};

  // This holds the self-signed certificate used by the server
  load_server_certificate(ctx);

  // Spawn a listening port
  asio::spawn(ioc, std::bind(
      &WsServerSSL::DoListen,
      this,
      std::ref(ioc),
      std::ref(ctx),
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

void WsServerSSL::OnFail(beast::error_code ec, char const *what) {
  if (options_.on_fail) {
    options_.on_fail(ec, what);
  }
}

// Accepts incoming connections and launches the sessions
void WsServerSSL::DoListen(
    asio::io_context &ioc,
    ssl::context &ctx,
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
      if (options_.http.enable) {
        asio::spawn(acceptor.get_executor(), std::bind(
            &WsServerSSL::DoSessionHTTP,
            this,
            beast::ssl_stream<beast::tcp_stream>(std::move(socket), ctx),
            std::placeholders::_1));
      } else {
        DoSessionWebSocket(
            websocket::stream<beast::ssl_stream<beast::tcp_stream>>(
                std::move(socket), ctx),
            boost::optional<http_req_t>(boost::none));
      }
    }
  }
}

// Handles an HTTP server connection
void WsServerSSL::DoSessionHTTP(
    tcp_stream_t &stream, asio::yield_context yield) {
  bool close = false;
  beast::error_code ec;

  // Set the timeout.
  beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

  // Perform the SSL handshake
  stream.async_handshake(ssl::stream_base::server, yield[ec]);
  if (ec)
    return OnFail(ec, "handshake");

  // This buffer is required to persist across reads
  beast::flat_buffer buffer;

  // The parser is stored in an optional container so we can
  // construct it from scratch it at the beginning of each new message.
  boost::optional<http::request_parser<http::string_body>> parser;

  // This lambda is used to send messages
  send_lambda_t lambda{stream, close, ec, yield};

  for (;;) {
    // Construct a new parser for each message
    parser.emplace();

    // Apply a reasonable limit to the allowed size
    // of the body in bytes to prevent abuse.
    parser->body_limit(10000);

    // Set the timeout.
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

    // Read a request
    http::async_read(stream, buffer, *parser, yield[ec]);
    if (ec == http::error::end_of_stream)
      break;
    if (ec)
      return OnFail(ec, "read");

    auto http_req = parser->release();

    // See if it is a WebSocket Upgrade
    if (websocket::is_upgrade(http_req)) {
      // Disable the timeout.
      // The websocket::stream uses its own timeout settings.
      beast::get_lowest_layer(stream).expires_never();

      DoSessionWebSocket(
          websocket::stream<beast::ssl_stream<beast::tcp_stream>>(
              std::move(stream)),
          boost::optional<http_req_t>(http_req));
      return;
    }

    // LOG(INFO) << "http req: " << http_req.target();
    auto handled = OnHandleHttpRequest(http_req, lambda);
    if (!handled) {
      net::handle_request(options_.http.doc_root, std::move(http_req), lambda);
      if (ec) return OnFail(ec, "write");
    }

    if (close) {
      // This means we should close the connection, usually because
      // the response indicated the "Connection: close" semantic.
      break;
    }
  }

  // Set the timeout.
  beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

  // Perform the SSL shutdown
  stream.async_shutdown(yield[ec]);
  if (ec) return OnFail(ec, "shutdown");

  // At this point the connection is closed gracefully
}

void WsServerSSL::DoSessionWebSocket(
    ws_stream_t &&ws, boost::optional<http_req_t> &&req) {
  (void)ws;
  (void)req;
  auto s = std::make_shared<WsSession<std::string>>(
      std::move(ws), std::move(req));
  s->SetEventCallback(net::NET_EVENT_FAIL,
      [this](const std::shared_ptr<WsSession<std::string>::event_t> &event) {
        auto e = std::dynamic_pointer_cast<net::NetFailEvent>(event);
        OnFail(e->ec, e->what.c_str());
      });
  // Echo back all received WebSocket messages
  s->SetEventCallback(net::NET_EVENT_RECV,
      [this, w = std::weak_ptr<WsSession<std::string>>(s)](
          const std::shared_ptr<WsSession<std::string>::event_t> &event) {
        auto e = std::dynamic_pointer_cast<net::NetRecvEvent>(event);
        auto buffer = e->buffer;
        auto bytes_n = e->bytes_n;
        if (auto s = w.lock())
          s->Send(std::make_shared<std::string>(
              beast::buffers_to_string(buffer.data())));
        buffer.consume(bytes_n);
      });
  s->Run();
}

bool WsServerSSL::OnHandleHttpRequest(
    http_req_t &req,
    send_lambda_t &send) {
  (void)req;
  (void)send;
  return false;
}
