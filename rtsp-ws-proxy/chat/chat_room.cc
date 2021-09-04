#include "chat_room.h"

#include <vector>

#include "common/util/log.h"

#include "chat_session.h"

ChatRoom::ChatRoom() {
  VLOG(2) << __func__;
}

ChatRoom::~ChatRoom() {
  VLOG(2) << __func__;
}

void ChatRoom::Join(const std::shared_ptr<ChatSession> &session) {
  std::lock_guard<std::mutex> lock(mutex_);
  sessions_.insert(session);
}

void ChatRoom::Leave(const std::shared_ptr<ChatSession> &session) {
  std::lock_guard<std::mutex> lock(mutex_);
  sessions_.erase(session);
}

void ChatRoom::Send(const std::shared_ptr<std::string> &data) {
  // Make a local list of all the weak pointers representing
  // the sessions, so we can do the actual sending without
  // holding the mutex:
  std::vector<std::weak_ptr<ChatSession>> v;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    v.reserve(sessions_.size());
    for (auto s : sessions_)
      v.emplace_back(std::weak_ptr<ChatSession>(s->shared_from_this()));
  }

  // For each session in our local list, try to acquire a strong
  // pointer. If successful, then send the message on that session.
  for (auto const &w : v)
    if (auto s = w.lock())
      s->Send(data);
}
