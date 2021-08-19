#include <glog/logging.h>

#include <condition_variable>
#include <mutex>

#include "common/util/options.h"
#include "http_client.h"
#include "ws_stream_client.h"

#define WS_JSON_STREAM_IGNORE
#include "common/util/json.h"

int main(int argc, char const *argv[]) {
  (void)argc;
  FLAGS_logtostderr = true;
  FLAGS_colorlogtostderr = true;
  FLAGS_minloglevel = google::INFO;
  FLAGS_v = 0;
  google::InitGoogleLogging(argv[0]);

  if (argc < 2) {
    LOG(ERROR) << "Usage: <program> config.yaml";
    return 1;
  }

  HttpClientOptions http_options{};
  WsClientOptions ws_options{};
  std::string ws_target_prefix = "/stream/";
  try {
    auto node = YAML::LoadFile(argv[1]);
    LOG(INFO) << "Load config success: " << argv[1];

    auto node_server = node["server"];
    if (node_server) {
      if (node_server["host"])
        http_options.host = ws_options.host =
            node_server["host"].as<std::string>();
      if (node_server["port"])
        http_options.port = ws_options.port = node_server["port"].as<int>();

      // http
      if (node_server["http"]) {
        auto node_http = node_server["http"];
        if (node_http["target"])
          http_options.target = node_http["target"].as<std::string>();
        if (node_http["timeout"])
          http_options.timeout = node_http["timeout"].as<int>();
      }

      // ws
      if (node_server["ws"]) {
        auto node_ws = node_server["ws"];
        if (node_ws["target_prefix"])
          ws_target_prefix = node_ws["target_prefix"].as<std::string>();
      }
    }
  } catch (const std::exception &e) {
    LOG(ERROR) << "Load config fail, " << e.what();
    return EXIT_FAILURE;
  }

  std::mutex mutex_http;
  std::condition_variable cond_http;
  int http_wait_secs = http_options.timeout;
  bool http_resp_ok = false;
  std::shared_ptr<ws::stream_infos_t> stream_infos = nullptr;

  auto on_fail = [](boost::beast::error_code ec, char const *what) {
    LOG(ERROR) << what << ": " << ec.message();
  };

  {
    http_options.on_fail = on_fail;
    LOG(INFO) << "HTTP get stream infos, http://" << http_options.host << ":"
        << http_options.port << http_options.target;
    HttpClient(http_options).Run(
        [&mutex_http, &cond_http, &http_resp_ok, &stream_infos](
            const HttpClient::response_t &res) {
          VLOG(1) << res;
          if (res.result() == boost::beast::http::status::ok) {
            std::unique_lock<std::mutex> lock(mutex_http);
            try {
              stream_infos = std::make_shared<ws::stream_infos_t>();
              *stream_infos = ws::to_stream_infos(
                  boost::beast::buffers_to_string(res.body().data()));
              http_resp_ok = true;
            } catch (std::exception &e) {
              LOG(ERROR) << " parse fail, " << e.what();
            }
            cond_http.notify_one();
          }
        });
  }
  {
    std::unique_lock<std::mutex> lock(mutex_http);
    cond_http.wait_for(lock,
        std::chrono::seconds(http_wait_secs),
        [&http_resp_ok]() { return http_resp_ok; });
  }
  if (http_resp_ok) {
    for (auto &&e : *stream_infos) {
      LOG(INFO) << ws_target_prefix << e.id;
      for (auto &&s : e.subs) {
        LOG(INFO) << " " << av_get_media_type_string(s.first);
        LOG(INFO) << "  " << ws::json(s.second);
      }
    }

    auto stream_info = stream_infos->front();

    ws_options.target = ws_target_prefix + stream_info.id;
    ws_options.on_fail = on_fail;

    WsStreamClient ws_client(ws_options);
    ws_client.Run();
  } else {
    LOG(ERROR) << "HTTP get stream infos timeout, > " << http_wait_secs << " s";
  }
  return EXIT_SUCCESS;
}
