#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>

template <typename T>
class Event {
 public:
  explicit Event(T id) : id(id) {}
  virtual ~Event() = default;
  T id;
};

template <typename T>
class EventManager {
 public:
  using event_t = Event<T>;
  using event_callback_t =
      std::function<void(const std::shared_ptr<event_t> &e)>;

  EventManager() = default;
  virtual ~EventManager() = default;

  bool HasEventCallback(T id) const {
    return event_callbacks_.find(id) != event_callbacks_.end();
  }

  void SetEventCallback(T id, event_callback_t cb) {
    if (cb == nullptr) {
      event_callbacks_.erase(id);
    } else {
      event_callbacks_[id] = cb;
    }
  }

 protected:
  void DispatchEvent(const std::shared_ptr<event_t> &e) {
    OnEvent(e);
    if (HasEventCallback(e->id)) {
      event_callbacks_[e->id](e);
    }
  }

  template <typename E, typename... Args>
  void DispatchEvent(Args&&... args) {
    DispatchEvent(std::make_shared<E>(std::forward<Args>(args)...));
  }

  virtual void OnEvent(const std::shared_ptr<event_t> &e) {
    (void)e;
  }

  std::unordered_map<T, event_callback_t> event_callbacks_;
};
