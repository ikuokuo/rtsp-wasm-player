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
#include <boost/version.hpp>

#include "common/util/options.h"
#include "ws_server.h"

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
  } catch (const std::exception &e) {
    LOG(ERROR) << "Load config fail, " << e.what();
    return EXIT_FAILURE;
  }

  WsServer server(options);
  server.Run();

  return EXIT_SUCCESS;
}
