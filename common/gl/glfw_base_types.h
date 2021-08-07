#pragma once

#include <functional>
#include <string>

class GlfwBase;
struct GLFWwindow;
class GlfwBaseCallback {
 public:
  virtual ~GlfwBaseCallback() = default;

  virtual void OnWindowBeforeCreate(GlfwBase *) {}
  virtual void OnWindowCreated(GlfwBase *, GLFWwindow *) {}

  virtual void OnGlfwInit(GlfwBase *) {}
  virtual void OnGlfwDraw(GlfwBase *) {}
  virtual void OnGlfwDestory(GlfwBase *) {}

  // Return true to override the original behavior
  virtual bool IsWindowBeforeCreateOverride(GlfwBase *) { return false; }
  virtual bool IsWindowCreatedOverride(GlfwBase *, GLFWwindow *) {
    return false;
  }
  virtual bool IsGlfwDrawOverride(GlfwBase *) { return false; }
};

struct GlfwInitParams {
  int width;
  int height;
  std::string title;
};

using GlfwRunCallback = std::function<void(GlfwBase *)>;
