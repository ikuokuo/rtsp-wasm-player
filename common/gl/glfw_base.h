#pragma once

#include <memory>
#include <string>

#include "glm/vec4.hpp"

#include "glfw_base_types.h"

struct GLFWwindow;

class GlfwBase {
 public:
  using Callback = std::shared_ptr<GlfwBaseCallback>;

  GlfwBase();
  virtual ~GlfwBase();

  GLFWwindow *Init(const GlfwInitParams &params = GlfwInitParams{});
  bool ShouldClose(bool accept_key_escape = true);
  void Draw();
  void Destroy();

  GLFWwindow *GetWindow() const;
  std::string GetGLSLVersion() const;

  void SetCallback(Callback callback);

  glm::vec4 clear_color() const { return clear_color_; }
  void set_clear_color(const glm::vec4 &color) { clear_color_ = color; }

  virtual int Run(
      const GlfwInitParams &params = GlfwInitParams{},
      GlfwRunCallback callback = nullptr);

 protected:
  virtual void OnWindowBeforeCreate();
  virtual void OnWindowCreated(GLFWwindow *);

  virtual void OnInit();
  virtual void OnDrawPre();
  virtual void OnDraw();
  virtual void OnDrawPost();
  virtual void OnDestroy();

 protected:
  GlfwInitParams DefaultInitParams(const GlfwInitParams &params);

  GLFWwindow *window_;
  std::string glsl_version_;

  Callback callback_;

  glm::vec4 clear_color_;
};
