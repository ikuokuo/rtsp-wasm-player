#include "glfw_base.h"

#include <cassert>
#include <iostream>

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GLFW/glfw3.h>

namespace {

void glfw_error_callback(int error, const char* description) {
  std::cerr << "Glfw Error " << error << ": " << description << std::endl;
}

void glfw_framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  (void)window;
  glViewport(0, 0, width, height);
}

}  // namespace

GlfwBase::GlfwBase()
  : window_(nullptr),
    callback_(nullptr),
    clear_color_(0.f, 0.f, 0.f, 1.f) {
}

GlfwBase::~GlfwBase() {
}

GLFWwindow *GlfwBase::Init(const GlfwInitParams &params_) {
  if (window_) return window_;
  auto params = DefaultInitParams(params_);

  // Setup window
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return nullptr;
  }

  if (callback_) {
    if (!callback_->IsWindowBeforeCreateOverride(this)) {
      OnWindowBeforeCreate();
    }
    callback_->OnWindowBeforeCreate(this);
  } else {
    OnWindowBeforeCreate();
  }

  // Create window with graphics context
  GLFWwindow* window = glfwCreateWindow(
      params.width, params.height, params.title.c_str(), NULL, NULL);
  if (window == NULL) {
    std::cerr << "Failed to open GLFW window" << std::endl;
    return nullptr;
  }

  window_ = window;
  if (callback_) {
    if (!callback_->IsWindowCreatedOverride(this, window)) {
      OnWindowCreated(window);
    }
    callback_->OnWindowCreated(this, window);
  } else {
    OnWindowCreated(window);
  }

  OnInit();
  if (callback_) callback_->OnGlfwInit(this);
  return window_;
}

bool GlfwBase::ShouldClose(bool accept_key_escape) {
  assert(window_);
  return glfwWindowShouldClose(window_) ||
      (accept_key_escape && glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS);
}

void GlfwBase::Draw() {
  assert(window_);
  if (callback_) {
    if (callback_->IsGlfwDrawOverride(this)) {
      callback_->OnGlfwDraw(this);
    } else {
      OnDrawPre();
      OnDraw();
      callback_->OnGlfwDraw(this);
      OnDrawPost();
    }
  } else {
    OnDrawPre();
    OnDraw();
    OnDrawPost();
  }
}

void GlfwBase::Destroy() {
  if (!window_) return;

  OnDestroy();
  if (callback_) callback_->OnGlfwDestory(this);

  glfwDestroyWindow(window_);
  glfwTerminate();
  window_ = nullptr;
}

GLFWwindow *GlfwBase::GetWindow() const {
  return window_;
}

std::string GlfwBase::GetGLSLVersion() const {
  return glsl_version_;
}

void GlfwBase::SetCallback(Callback callback) {
  callback_ = callback;
}

int GlfwBase::Run(const GlfwInitParams &params, GlfwRunCallback callback) {
  GLFWwindow *glfw_window = Init(params);
  if (!glfw_window) return 1;

  while (!ShouldClose()) {
    if (callback) callback(this);
    Draw();
  }

  Destroy();
  return 0;
}

void GlfwBase::OnWindowBeforeCreate() {
  // Decide GL+GLSL versions
  glfwWindowHint(GLFW_SAMPLES, 4);
#if __APPLE__
  // GL 3.2 + GLSL 150
  const char* glsl_version = "#version 150";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
  // GL 3.0 + GLSL 130
  const char* glsl_version = "#version 130";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
  // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif
  glsl_version_ = glsl_version;
}

void GlfwBase::OnWindowCreated(GLFWwindow *window) {
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);
  glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
  glfwSwapInterval(1);  // Enable vsync
}

void GlfwBase::OnInit() {
}

void GlfwBase::OnDrawPre() {
  glClearColor(clear_color_.r, clear_color_.g, clear_color_.b, clear_color_.a);
  glClear(GL_COLOR_BUFFER_BIT);
}

void GlfwBase::OnDraw() {
}

void GlfwBase::OnDrawPost() {
  glfwSwapBuffers(window_);
  // Poll and handle events (inputs, window resize, etc.)
  // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
  // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
  // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
  // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
  glfwPollEvents();
}

void GlfwBase::OnDestroy() {
}

GlfwInitParams GlfwBase::DefaultInitParams(const GlfwInitParams &params_) {
  GlfwInitParams params = params_;
  if (params.width <= 0) params.width = 1280;
  if (params.height <= 0) params.height = 720;
  if (params.title.empty()) params.title = "Window";
  return params;
}
