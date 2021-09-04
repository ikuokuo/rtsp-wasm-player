#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/version.h>
#include <libavdevice/version.h>
#include <libavformat/version.h>
#include <libavutil/version.h>
#include <libswscale/version.h>

#ifdef __cplusplus
}
#endif

#include <map>
#include <vector>

#include <boost/version.hpp>

#define UTIL_CONFIG_STREAM
#define UTIL_CONFIG_CORS
#include "common/util/config.h"
#include "common/util/log.h"

#include "stream_handler.h"
#include "stream_player.h"
#include "ws_stream_server.h"

struct Config {
  WsServerOptions options{};
  std::map<std::string, StreamOptions> stream_options;
  int stream_get_frequency = 20;
  bool stream_ui_enable = false;
};
int LoadConfig(const std::string &path, Config *config);

int main(int argc, char const *argv[]) {
  config::InitGoogleLoggingFlags();
  google::InitGoogleLogging(argv[0]);

  LOG(INFO) << "boost version: " << BOOST_VERSION;
  LOG(INFO) << "ffmpeg version";
  LOG(INFO) << "  libavcodec: " << AV_STRINGIFY(LIBAVCODEC_VERSION);
  LOG(INFO) << "  libavdevice: " << AV_STRINGIFY(LIBAVDEVICE_VERSION);
  LOG(INFO) << "  libavformat: " << AV_STRINGIFY(LIBAVFORMAT_VERSION);
  LOG(INFO) << "  libavutil: " << AV_STRINGIFY(LIBAVUTIL_VERSION);
  LOG(INFO) << "  libswscale: " << AV_STRINGIFY(LIBSWSCALE_VERSION);

  if (argc < 2) {
    LOG(ERROR) << "Usage: <program> config.yaml";
    return 1;
  }

  Config config{};
  auto ret = LoadConfig(argv[1], &config);
  if (ret != EXIT_SUCCESS) {
    return ret;
  }

  config.options.on_fail = [](boost::beast::error_code ec, char const *what) {
    LOG(ERROR) << what << ": " << ec.message();
  };
  WsStreamServer server(config.options);

  std::vector<std::shared_ptr<StreamHandler>> streams;
  std::unordered_map<std::string, std::shared_ptr<StreamPlayer>> players;
  for (auto &&entry : config.stream_options) {
    auto id = entry.first;
    std::shared_ptr<StreamPlayer> player = nullptr;
    if (config.stream_ui_enable) {
      player = std::make_shared<StreamPlayer>(id);
      player->Start();
      players[id] = player;
    }
    auto stream = std::make_shared<StreamHandler>(
      id, entry.second, config.stream_get_frequency,
      [id, &server, player](
          const std::shared_ptr<Stream> &stream,
          const AVMediaType &type, AVPacket *packet) {
        server.Send(id, stream, type, packet);
        if (player != nullptr) {
          player->Send(id, stream, type, packet);
        }
      });
    stream->Start();
    streams.push_back(stream);
  }

  server.Run();

  for (auto &&s : streams)
    s->Stop();
  for (auto &&p : players)
    p.second->Stop();
  return EXIT_SUCCESS;
}

int LoadConfig(const std::string &path, Config *config) {
  LOG(INFO) << "Load config: " << path;
  auto &options = config->options;
  auto &stream_options = config->stream_options;
  auto &stream_get_frequency = config->stream_get_frequency;
  auto &stream_ui_enable = config->stream_ui_enable;
  try {
    auto node = YAML::LoadFile(path);
    config::InitGoogleLoggingFlags(node["log"]);

    if (node["server"]) {
      auto node_server = node["server"];
      if (node_server["addr"])
        options.addr = node_server["addr"].as<std::string>();
      if (node_server["port"])
        options.port = node_server["port"].as<int>();
      if (node_server["threads"])
        options.threads = node_server["threads"].as<int>();

      if (node_server["http"]) {
        auto node_http = node_server["http"];
        if (node_http["enable"])
          options.http.enable = node_http["enable"].as<bool>();
        if (node_http["doc_root"])
          options.http.doc_root = node_http["doc_root"].as<std::string>();
      }

      if (node_server["cors"])
        options.cors = node_server["cors"].as<net::Options>();

      if (node_server["stream"]) {
        auto node_stream = node_server["stream"];
        if (node_stream["http_target"])
          options.stream.http_target =
              node_stream["http_target"].as<std::string>();
        if (node_stream["ws_target_prefix"])
          options.stream.ws_target_prefix =
              node_stream["ws_target_prefix"].as<std::string>();
      }
    }

    if (node["streams"]) {
      auto node_streams = node["streams"];
      for (auto it = node_streams.begin(); it != node_streams.end(); ++it) {
        auto id = (*it)["id"].as<std::string>();
        stream_options[id] = it->as<StreamOptions>();
      }
    }

    if (node["stream_get_frequency"])
      stream_get_frequency = node["stream_get_frequency"].as<int>();

    if (node["stream_ui_enable"])
      stream_ui_enable = node["stream_ui_enable"].as<bool>();
  } catch (const std::exception &e) {
    LOG(ERROR) << "Load config fail, " << e.what();
    return EXIT_FAILURE;
  }

  if (stream_options.empty()) {
    LOG(ERROR) << "Streams is empty!";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
