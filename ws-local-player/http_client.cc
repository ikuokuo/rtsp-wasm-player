#include "http_client.h"

#include <boost/beast/version.hpp>

HttpClient::HttpClient(const HttpClientOptions &options)
  : options_(options), run_response_cb_(nullptr) {
}

HttpClient::~HttpClient() {
}

void HttpClient::Run(on_response_t cb) {
  run_response_cb_ = cb;

  // The io_context is required for all I/O
  asio::io_context ioc;

  // Launch the asynchronous operation
  asio::spawn(ioc, std::bind(
      &HttpClient::DoSession,
      this,
      std::ref(ioc),
      std::placeholders::_1));

  // Run the I/O service. The call will return when
  // the get operation is complete.
  ioc.run();
}

void HttpClient::OnFail(beast::error_code ec, char const *what) {
  if (options_.on_fail) {
    options_.on_fail(ec, what);
  }
}

void HttpClient::DoSession(asio::io_context &ioc, asio::yield_context yield) {
  beast::error_code ec;

  // These objects perform our I/O
  tcp::resolver resolver(ioc);
  beast::tcp_stream stream(ioc);

  auto host = options_.host;
  auto port = std::to_string(options_.port);

  // Look up the domain name
  auto const results = resolver.async_resolve(host, port, yield[ec]);
  if (ec) return OnFail(ec, "resolve");

  // Set the timeout.
  stream.expires_after(std::chrono::seconds(options_.timeout));

  // Make the connection on the IP address we get from a lookup
  stream.async_connect(results, yield[ec]);
  if (ec) return OnFail(ec, "connect");

  // Set up an HTTP GET request message
  http::request<http::string_body> req{http::verb::get,
      options_.target, options_.version};
  req.set(http::field::host, host);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

  // Set the timeout.
  stream.expires_after(std::chrono::seconds(options_.timeout));

  // Send the HTTP request to the remote host
  http::async_write(stream, req, yield[ec]);
  if (ec) return OnFail(ec, "write");

  // This buffer is used for reading and must be persisted
  beast::flat_buffer b;

  // Declare a container to hold the response
  http::response<http::dynamic_body> res;

  // Receive the HTTP response
  http::async_read(stream, b, res, yield[ec]);
  if (ec) return OnFail(ec, "read");

  NotifyResponse(res);

  // Gracefully close the socket
  stream.socket().shutdown(tcp::socket::shutdown_both, ec);

  // not_connected happens sometimes
  // so don't bother reporting it.
  //
  if (ec && ec != beast::errc::not_connected)
    return OnFail(ec, "shutdown");

  // If we get here then the connection is closed gracefully
}

void HttpClient::OnResponse(const response_t &res) {
  (void)res;
}

void HttpClient::NotifyResponse(const response_t &res) {
  OnResponse(res);
  if (run_response_cb_) {
    run_response_cb_(res);
  }
  if (options_.on_response) {
    options_.on_response(res);
  }
}
