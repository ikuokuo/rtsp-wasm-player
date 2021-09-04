#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class WsStreamSession;

class WsStreamRoom {
 public:
  using data_t = std::vector<uint8_t>;
  using sessions_set_t = std::unordered_set<std::shared_ptr<WsStreamSession>>;

  WsStreamRoom();
  ~WsStreamRoom();

  void Join(const std::string &id,
      const std::shared_ptr<WsStreamSession> &session);
  void Leave(const std::string &id,
      const std::shared_ptr<WsStreamSession> &session);

  void Send(const std::string &id,
      const std::shared_ptr<data_t> &data);

 private:
  std::unordered_map<std::string, sessions_set_t> sessions_map_;

  std::mutex mutex_;
};
