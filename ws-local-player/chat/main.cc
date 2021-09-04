#include <functional>
#include <iostream>
#include <thread>

#include <boost/asio/signal_set.hpp>
#include <boost/lexical_cast.hpp>

#include "common/util/config.h"
#include "common/util/log.h"
#include "common/util/strings.h"
#include "../ws_client.h"

namespace ansi {

constexpr auto kRed     = "\033[31m";
constexpr auto kGreen   = "\033[32m";
constexpr auto kYellow  = "\033[33m";
constexpr auto kBlue    = "\033[34m";
constexpr auto kMagenta = "\033[35m";

constexpr auto kRedBold     = "\033[1;31m";
constexpr auto kGreenBold   = "\033[1;32m";
constexpr auto kYellowBold  = "\033[1;33m";
constexpr auto kBlueBold    = "\033[1;34m";
constexpr auto kMagentaBold = "\033[1;35m";

constexpr auto kEnd = "\033[m";
constexpr auto kErase = "\r\033[K";

}  // namespace ansi

class ChatClient : public WsClient<std::string> {
 public:
  using on_recv_t = std::function<void(const std::string &)>;

  ChatClient(asio::io_context &ioc, const WsClientOptions &options)
    : WsClient<std::string>(ioc, options) {
  }
  ~ChatClient() {
  }

 protected:
  void OnEventOpened() override {
    auto ep = ws_.next_layer().socket().local_endpoint();
    LOG(WARNING) << "I'm [" << boost::lexical_cast<std::string>(ep) << "]";
  }

  void OnEventRecv(beast::flat_buffer &buffer, std::size_t bytes_n) override {
    WsClient<std::string>::OnEventRecv(buffer, bytes_n);
    auto s = beast::buffers_to_string(buffer.data());
    std::cout << ansi::kGreen << ansi::kErase << s << ansi::kEnd << std::endl;
    std::cout << ansi::kMagentaBold << "Say: " << ansi::kEnd << std::flush;
    buffer.consume(bytes_n);
  }
};

int main(int argc, char const *argv[]) {
  config::InitGoogleLoggingFlags();
  google::InitGoogleLogging(argv[0]);

  if (argc < 2) {
    LOG(ERROR) << "Usage: <program> config.yaml";
    return 1;
  }

  LOG(INFO) << "Load config: " << argv[1];
  WsClientOptions options{};
  try {
    auto node = YAML::LoadFile(argv[1]);
    config::InitGoogleLoggingFlags(node["log"]);

    if (node["server"]) {
      auto node_server = node["server"];
      if (node_server["host"])
        options.host = node_server["host"].as<std::string>();
      if (node_server["port"])
        options.port = node_server["port"].as<int>();
    }
  } catch (const std::exception &e) {
    LOG(ERROR) << " parse options fail, " << e.what();
    return 1;
  }

  std::atomic_bool stop{false};
  asio::io_context ioc;
  asio::signal_set signals(ioc, SIGINT, SIGTERM);
  signals.async_wait([&ioc, &stop](beast::error_code const&, int) {
    stop = true;
    ioc.stop();
  });
  std::thread t([&ioc]() {
    ioc.run();
  });

  auto client = std::make_shared<ChatClient>(ioc, options);
  client->SetEventCallback(net::NET_EVENT_FAIL,
      [&stop](const std::shared_ptr<ChatClient::event_t> &event) {
        auto e = std::dynamic_pointer_cast<net::NetFailEvent>(event);
        LOG(ERROR) << e->what << ": " << e->ec.message();
        stop = true;
      });
  client->SetEventCallback(net::NET_EVENT_CLOSED,
      [&ioc, &stop](const std::shared_ptr<ChatClient::event_t> &) {
        LOG(INFO) << "closed";
        stop = true;
        ioc.stop();
      });
  client->Open();

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  while (!stop.load()) {
    std::string word;
    std::cout << ansi::kMagentaBold << "Say: " << ansi::kEnd << std::flush;
    std::getline(std::cin, word);
    if (!std::cin) break;
    strings::trim(word);
    if (!word.empty()) {
      client->Send(std::make_shared<std::string>(std::move(word)));
    } else {
      // break;
    }
  }

  client->Close();
  t.join();
  return EXIT_SUCCESS;
}
