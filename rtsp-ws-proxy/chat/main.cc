#include "common/util/config.h"
#include "common/util/log.h"

#include "chat_server.h"

int main(int argc, char const *argv[]) {
  config::InitLogging(argv[0]);

  if (argc < 2) {
    LOG(ERROR) << "Usage: <program> config.yaml";
    return 1;
  }

  LOG(INFO) << "Load config: " << argv[1];
  WsServerOptions options{};
  try {
    auto node = YAML::LoadFile(argv[1]);
    config::InitLoggingFlags(node["log"]);

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
    }
  } catch (const std::exception &e) {
    LOG(ERROR) << " parse options fail, " << e.what();
    return 1;
  }

  options.on_fail = [](boost::beast::error_code ec, char const *what) {
    LOG(ERROR) << what << ": " << ec.message();
  };

  auto server = std::make_shared<ChatServer>(options);
  server->Run();

  return EXIT_SUCCESS;
}
