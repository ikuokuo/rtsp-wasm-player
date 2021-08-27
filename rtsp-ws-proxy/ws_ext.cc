#include "ws_ext.h"

#include <thread>

#include <boost/asio/signal_set.hpp>

#include "common/util/log.h"

namespace ws_ext {

// Report a failure
void fail(boost::beast::error_code ec, char const *what) {
  LOG(ERROR) << what << ": " << ec.message();
}

// Return a reasonable mime type based on the extension of a file.
beast::string_view mime_type(beast::string_view path) {
  using beast::iequals;
  auto const ext = [&path] {
    auto const pos = path.rfind(".");
    if (pos == beast::string_view::npos) {
      return beast::string_view{};
    }
    return path.substr(pos);
  }();
  if (iequals(ext, ".htm"))  return "text/html";
  if (iequals(ext, ".html")) return "text/html";
  if (iequals(ext, ".php"))  return "text/html";
  if (iequals(ext, ".css"))  return "text/css";
  if (iequals(ext, ".txt"))  return "text/plain";
  if (iequals(ext, ".js"))   return "application/javascript";
  if (iequals(ext, ".json")) return "application/json";
  if (iequals(ext, ".xml"))  return "application/xml";
  if (iequals(ext, ".swf"))  return "application/x-shockwave-flash";
  if (iequals(ext, ".flv"))  return "video/x-flv";
  if (iequals(ext, ".png"))  return "image/png";
  if (iequals(ext, ".jpe"))  return "image/jpeg";
  if (iequals(ext, ".jpeg")) return "image/jpeg";
  if (iequals(ext, ".jpg"))  return "image/jpeg";
  if (iequals(ext, ".gif"))  return "image/gif";
  if (iequals(ext, ".bmp"))  return "image/bmp";
  if (iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
  if (iequals(ext, ".tiff")) return "image/tiff";
  if (iequals(ext, ".tif"))  return "image/tiff";
  if (iequals(ext, ".svg"))  return "image/svg+xml";
  if (iequals(ext, ".svgz")) return "image/svg+xml";
  return "application/text";
}

// Append an HTTP rel-path to a local filesystem path.
// The returned path is normalized for the platform.
std::string path_cat(beast::string_view base, beast::string_view path) {
  if (base.empty()) return std::string(path);
  std::string result(base);
#ifdef BOOST_MSVC
  char constexpr path_separator = '\\';
  if (result.back() == path_separator) {
    result.resize(result.size() - 1);
  }
  result.append(path.data(), path.size());
  for (auto &c : result) {
    if (c == '/') c = path_separator;
  }
#else
  char constexpr path_separator = '/';
  if (result.back() == path_separator) {
    result.resize(result.size() - 1);
  }
  result.append(path.data(), path.size());
#endif
  return result;
}

void run_server(const boost::asio::ip::tcp::endpoint &endpoint, int threads,
                const std::string &doc_root) {
  // The io_context is required for all I/O
  asio::io_context ioc(threads);

  // Create and launch a listening port
  std::make_shared<ws_ext::listener>(
      ioc,
      endpoint,
      doc_root)->run();

  // Capture SIGINT and SIGTERM to perform a clean shutdown
  asio::signal_set signals(ioc, SIGINT, SIGTERM);
  signals.async_wait([&](beast::error_code const&, int) {
    // Stop the `io_context`. This will cause `run()`
    // to return immediately, eventually destroying the
    // `io_context` and all of the sockets in it.
    ioc.stop();
  });

  // Run the I/O service on the requested number of threads
  std::vector<std::thread> v;
  v.reserve(threads - 1);
  for (auto i = threads - 1; i > 0; --i) {
    v.emplace_back([&ioc] {
      ioc.run();
    });
  }
  ioc.run();

  // (If we get here, it means we got a SIGINT or SIGTERM)

  // Block until all the threads exit
  for (auto &t : v)
    t.join();
}

}  // namespace ws_ext
