#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>

class ChatSession;

class ChatRoom {
 public:
  ChatRoom();
  ~ChatRoom();

  void Join(const std::shared_ptr<ChatSession> &session);
  void Leave(const std::shared_ptr<ChatSession> &session);
  void Send(const std::shared_ptr<std::string> &data);

 private:
  std::unordered_set<std::shared_ptr<ChatSession>> sessions_;

  std::mutex mutex_;
};
