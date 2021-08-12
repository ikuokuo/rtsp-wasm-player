#include <glog/logging.h>

#include "common/util/options.h"
#include "ws_client.h"

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

  std::string host = "127.0.0.1";
  int port = 8080;

  try {
    auto node = YAML::LoadFile(argv[1]);
    LOG(INFO) << "Load config success: " << argv[1];

    auto node_server = node["server"];
    if (node_server) {
      host = node_server["host"].as<std::string>();
      port = node_server["port"].as<int>();
    }
  } catch (const std::exception &e) {
    LOG(INFO) << "Load config fail, " << e.what();
    return EXIT_FAILURE;
  }

  WsClient client(host, port);
  client.Run();

  return EXIT_SUCCESS;
}
