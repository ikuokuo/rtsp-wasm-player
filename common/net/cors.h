#pragma once

#include <algorithm>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <glog/logging.h>

namespace net {

namespace beast = boost::beast;
namespace http = beast::http;

struct Options {
  bool enabled;
  std::vector<std::string> allowed_origins;
  std::vector<std::string> allowed_methods;
  std::vector<std::string> allowed_headers;
  std::vector<std::string> exposed_headers;
  bool allowed_credentials;
  int max_age;
  bool debug;
};

// Ref: https://github.com/rs/cors/
template <typename Body = http::string_body,
          typename Allocator = std::allocator<char>>
class Cors {
 public:
  explicit Cors(const Options &options)
    : options_(options),
      allowed_origins_all_(
          std::find(options.allowed_origins.begin(),
                    options.allowed_origins.end(), "*")
              != options.allowed_origins.end()),
      allowed_headers_all_(
          std::find(options.allowed_headers.begin(),
                    options.allowed_headers.end(), "*")
              != options.allowed_headers.end()) {
  }
  ~Cors() = default;

  bool operator()(
      const http::request<Body, http::basic_fields<Allocator>> &req,
      http::response<Body> &res) {
    return Handle(req, res);
  }

  bool Handle(
      const http::request<Body, http::basic_fields<Allocator>> &req,
      http::response<Body> &res) {
    if (!options_.enabled) return false;

    auto const no_content = [&req]() {
      http::response<http::string_body> res{
          http::status::no_content, req.version()};
      res.keep_alive(req.keep_alive());
      res.prepare_payload();
      return res;
    };

    LOG_IF(INFO, options_.debug) << "cors Handle, method: "
        << http::to_string(req.method());
    if (req.method() == http::verb::options &&
        req[http::field::access_control_request_method] != "") {
      HandlePreflight(req, res);
      res = no_content();
      return true;
    } else {
      HandleRequest(req, res);
      return false;
    }
  }

 private:
  void HandlePreflight(
      const http::request<Body, http::basic_fields<Allocator>> &req,
      http::response<Body> &res) {
    res.insert(http::field::vary, http::to_string(http::field::origin));
    res.insert(http::field::vary,
        http::to_string(http::field::access_control_request_method));
    res.insert(http::field::vary,
        http::to_string(http::field::access_control_request_headers));

    auto origin = req[http::field::origin];
    LOG_IF(INFO, options_.debug) << "cors HandlePreflight, origin: " << origin;
    if (origin == "") {
      LOG_IF(WARNING, options_.debug) << "  preflight aborted, origin is empty";
      return;
    }
    if (!IsOriginAllowed(origin)) {
      LOG_IF(WARNING, options_.debug)
          << "  preflight aborted, origin not allowed: " << origin;
      return;
    }

    auto method = req[http::field::access_control_request_method];
    if (!IsMethodAllowed(method)) {
      LOG_IF(WARNING, options_.debug)
          << "  preflight aborted, method not allowed: " << method;
      return;
    }

    auto headers = req[http::field::access_control_request_headers];
    if (!AreHeadersAllowed(headers)) {
      LOG_IF(WARNING, options_.debug)
          << "  preflight aborted, headers not allowed: " << headers;
      return;
    }

    if (allowed_origins_all_) {
      res.set(http::field::access_control_allow_origin, "*");
    } else {
      res.set(http::field::access_control_allow_origin, origin);
    }
    res.set(http::field::access_control_allow_methods, method);
    res.set(http::field::access_control_allow_headers, headers);
    if (options_.allowed_credentials) {
      res.set(http::field::access_control_allow_credentials, "true");
    }
    if (options_.max_age > 0) {
      res.set(http::field::access_control_max_age,
          std::to_string(options_.max_age));
    }
  }

  void HandleRequest(
      const http::request<Body, http::basic_fields<Allocator>> &req,
      http::response<Body> &res) {
    res.insert(http::field::vary, http::to_string(http::field::origin));

    auto origin = req[http::field::origin];
    LOG_IF(INFO, options_.debug) << "cors HandleRequest, origin: " << origin;
    if (origin == "") {
      LOG_IF(WARNING, options_.debug) << "  request aborted, origin is empty";
      return;
    }
    if (!IsOriginAllowed(origin)) {
      LOG_IF(WARNING, options_.debug)
          << "  request aborted, origin not allowed: " << origin;
      return;
    }

    auto method = http::to_string(req.method());
    if (!IsMethodAllowed(method)) {
      LOG_IF(WARNING, options_.debug)
          << "  request aborted, method not allowed: " << method;
      return;
    }

    if (allowed_origins_all_) {
      res.set(http::field::access_control_allow_origin, "*");
    } else {
      res.set(http::field::access_control_allow_origin, origin);
    }
    if (options_.exposed_headers.size() > 0) {
      res.set(http::field::access_control_expose_headers,
          boost::algorithm::join(options_.exposed_headers, ", "));
    }
    if (options_.allowed_credentials) {
      res.set(http::field::access_control_allow_credentials, "true");
    }
  }

  bool IsOriginAllowed(const boost::string_view &origin_) {
    if (allowed_origins_all_) return true;
    if (options_.allowed_origins.size() < 0) return false;
    auto origin = origin_.to_string();
    for (auto &&s : options_.allowed_origins) {
      if (s == origin) return true;
    }
    for (auto &&s : options_.allowed_origins) {
      std::smatch m;
      if (std::regex_match(origin, std::regex(s))) return true;
    }
    return false;
  }

  bool IsMethodAllowed(const boost::string_view &method_) {
    if (options_.allowed_methods.size() < 0) return false;
    if (method_ == http::to_string(http::verb::options)) {
      // Always allow preflight requests
      return true;
    }
    auto method = method_.to_string();
    for (auto &&s : options_.allowed_methods) {
      if (s == method) return true;
    }
    return false;
  }

  bool AreHeadersAllowed(const boost::string_view &headers_) {
    std::vector<std::string> headers;
    boost::algorithm::split(headers, headers_, boost::is_any_of(", "));
    for (auto &&s : options_.allowed_headers) {
      if (std::find(headers.begin(), headers.end(), s) == headers.end()) {
        return false;
      }
    }
    return true;
  }

  Options options_;
  bool allowed_origins_all_;
  bool allowed_headers_all_;
};

}  // namespace net
