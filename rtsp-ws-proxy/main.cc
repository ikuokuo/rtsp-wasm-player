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

#include <glog/logging.h>

#include <map>
#include <vector>

#include <boost/version.hpp>

#include "common/util/options.h"
#include "stream_handler.h"
#include "stream_player.h"
#include "ws_stream_server.h"

int main(int argc, char const *argv[]) {
  (void)argc;
  FLAGS_logtostderr = true;
  FLAGS_colorlogtostderr = true;
  FLAGS_minloglevel = google::INFO;
  FLAGS_v = 0;
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

  WsServerOptions options{};
  std::map<std::string, StreamOptions> stream_options;
  int stream_get_frequency = 20;
  bool stream_ui_enable = false;
  try {
    auto node = YAML::LoadFile(argv[1]);
    LOG(INFO) << "Load config success: " << argv[1];

    auto node_server = node["server"];
    if (node_server) {
      if (node_server["addr"])
        options.addr = node_server["addr"].as<std::string>();
      if (node_server["port"])
        options.port = node_server["port"].as<int>();
      if (node_server["threads"])
        options.threads = node_server["threads"].as<int>();
      if (node_server["http_enable"])
        options.http_enable = node_server["http_enable"].as<bool>();
      if (node_server["http_doc_root"])
        options.http_doc_root = node_server["http_doc_root"].as<std::string>();
    }

    auto node_streams = node["streams"];
    if (node_streams) {
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

  WsStreamServer server(options);

  std::vector<std::shared_ptr<StreamHandler>> streams;
  std::unordered_map<std::string, std::shared_ptr<StreamPlayer>> players;
  for (auto &&entry : stream_options) {
    auto id = entry.first;
    std::shared_ptr<StreamPlayer> player = nullptr;
    if (stream_ui_enable) {
      player = std::make_shared<StreamPlayer>(id);
      player->Start();
      players[id] = player;
    }
    auto stream = std::make_shared<StreamHandler>(
      id, entry.second, stream_get_frequency,
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
