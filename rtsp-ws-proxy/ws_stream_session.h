#pragma once

#include <memory>
#include <string>
#include <vector>

#include "common/util/ptr.h"

#include "ws_session.h"

class WsStreamRoom;

class WsStreamSession
  : public WsSession<std::vector<uint8_t>>,
    public virtual_enable_shared_from_this<WsStreamSession> {
 public:
  using virtual_enable_shared_from_this<WsStreamSession>::shared_from_this;

  WsStreamSession(ws_stream_t &&ws, boost::optional<http_req_t> &&req,
      std::string id, std::shared_ptr<WsStreamRoom> room);
  ~WsStreamSession() override;

 protected:
  void OnEventOpened() override;
  void OnEventClosed() override;

  void OnEventSend(std::shared_ptr<void> data) override;

  std::string id_;
  std::shared_ptr<WsStreamRoom> room_;
  std::string who_;
};
