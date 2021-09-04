#include "ws_stream_room.h"

#include "common/util/log.h"

#include "ws_stream_session.h"

WsStreamRoom::WsStreamRoom() {
  VLOG(2) << __func__;
}

WsStreamRoom::~WsStreamRoom() {
  VLOG(2) << __func__;
}

void WsStreamRoom::Join(const std::string &id,
    const std::shared_ptr<WsStreamSession> &session) {
  std::lock_guard<std::mutex> lock(mutex_);
  sessions_map_[id].insert(session);
}

void WsStreamRoom::Leave(const std::string &id,
    const std::shared_ptr<WsStreamSession> &session) {
  std::lock_guard<std::mutex> lock(mutex_);
  sessions_map_[id].erase(session);
}

void WsStreamRoom::Send(const std::string &id,
    const std::shared_ptr<data_t> &data) {
  std::vector<std::weak_ptr<WsStreamSession>> v;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    v.reserve(sessions_map_[id].size());
    for (auto p : sessions_map_[id])
      v.emplace_back(std::weak_ptr<WsStreamSession>(p->shared_from_this()));
  }

  for (auto const &w : v)
    if (auto s = w.lock())
      s->Send(data);
}
