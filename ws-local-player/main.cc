#include <condition_variable>
#include <mutex>

#include <boost/asio/signal_set.hpp>

#include "common/util/config.h"
#include "http_client.h"
#include "ws_stream_client.h"

#define NET_JSON_STREAM_IGNORE
#include "common/net/json.h"
#include "common/util/log.h"

int main(int argc, char const *argv[]) {
  config::InitLogging(argv[0]);

  if (argc < 2) {
    LOG(ERROR) << "Usage: <program> config.yaml [stream_id]";
    return 1;
  }

  HttpClientOptions http_options{};
  WsStreamClientOptions client_options{};
  std::string ws_target_prefix = "/stream/";
  std::string ws_target_id;
  LOG(INFO) << "Load config: " << argv[1];
  try {
    auto node = YAML::LoadFile(argv[1]);
    config::InitLoggingFlags(node["log"]);

    if (node["server"]) {
      auto node_server = node["server"];
      if (node_server["host"])
        http_options.host = client_options.ws.host =
            node_server["host"].as<std::string>();
      if (node_server["port"])
        http_options.port = client_options.ws.port =
            node_server["port"].as<int>();

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
        if (node_ws["target_id"])
          ws_target_id = node_ws["target_id"].as<std::string>();
      }
    }

    if (node["client"]) {
      auto node_client = node["client"];
      if (node_client["codec_thread_count"])
        client_options.codec_thread_count =
            node_client["codec_thread_count"].as<int>();
      if (node_client["codec_thread_type"])
        client_options.codec_thread_type =
            node_client["codec_thread_type"].as<int>();
      if (node_client["ui_wait_secs"])
        client_options.ui_wait_secs = node_client["ui_wait_secs"].as<int>();
    }

    if (argc >= 3)
      ws_target_id = argv[2];
  } catch (const std::exception &e) {
    LOG(ERROR) << "Load config fail, " << e.what();
    return EXIT_FAILURE;
  }

  std::mutex http_mutex;
  std::condition_variable http_cond;
  int http_wait_secs = http_options.timeout;
  bool http_resp_ok = false;
  bool http_fail = false;
  std::shared_ptr<net::stream_infos_t> stream_infos = nullptr;

  {
    http_options.on_fail = [&http_mutex, &http_cond, &http_fail](
        beast::error_code ec, char const *what) {
      LOG(ERROR) << what << ": " << ec.message();
      std::unique_lock<std::mutex> lock(http_mutex);
      http_fail = true;
      http_cond.notify_one();
    };
    LOG(INFO) << "HTTP get stream infos, http://" << http_options.host << ":"
        << http_options.port << http_options.target;
    HttpClient(http_options).Run(
        [&http_mutex, &http_cond, &http_resp_ok, &stream_infos](
            const HttpClient::response_t &res) {
          VLOG(1) << res;
          if (res.result() == beast::http::status::ok) {
            std::unique_lock<std::mutex> lock(http_mutex);
            try {
              stream_infos = std::make_shared<net::stream_infos_t>();
              *stream_infos = net::to_stream_infos(
                  beast::buffers_to_string(res.body().data()));
              http_resp_ok = true;
            } catch (std::exception &e) {
              LOG(ERROR) << " parse fail, " << e.what();
            }
            http_cond.notify_one();
          }
        });
  }
  {
    std::unique_lock<std::mutex> lock(http_mutex);
    http_cond.wait_for(lock,
        std::chrono::seconds(http_wait_secs),
        [&http_resp_ok, &http_fail]() { return http_resp_ok || http_fail; });
  }
  if (http_resp_ok) {
    int stream_info_index = -1;
    for (int i = 0, n = stream_infos->size(); i < n; ++i) {
      auto &&stream_info = (*stream_infos)[i];
      if (stream_info.id == ws_target_id) {
        stream_info_index = i;
      }
      LOG(INFO) << ws_target_prefix << stream_info.id;
      for (auto &&s : stream_info.subs) {
        LOG(INFO) << " " << av_get_media_type_string(s.first);
        LOG(INFO) << "  " << net::json(s.second);
      }
    }

    if (stream_info_index == -1) {
      LOG(WARNING) << "WS stream not found, id=" << ws_target_id;
      return EXIT_SUCCESS;
    }

    auto stream_info = (*stream_infos)[stream_info_index];

    asio::io_context ioc;
    asio::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait([&ioc](beast::error_code const&, int) {
      ioc.stop();
    });

    client_options.ws.target = ws_target_prefix + stream_info.id;
    client_options.stream_info = stream_info;
    client_options.ui_exit_func = [&ioc]() {
      ioc.stop();
    };
    auto client = std::make_shared<WsStreamClient>(
        ioc, client_options);
    client->SetEventCallback(net::NET_EVENT_FAIL,
        [](const std::shared_ptr<WsStreamClient::event_t> &event) {
          auto e = std::dynamic_pointer_cast<net::NetFailEvent>(event);
          LOG(ERROR) << e->what << ": " << e->ec.message();
        });
    client->Open();

    ioc.run();
  } else if (http_fail) {
    LOG(ERROR) << "HTTP get fail";
  } else {
    LOG(ERROR) << "HTTP get stream infos timeout, > " << http_wait_secs << " s";
  }
  return EXIT_SUCCESS;
}
