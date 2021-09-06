#pragma once

#include <memory>

#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengles2.h>

#include "common/gl/shader.h"
#include "common/gl/texture.h"

#include "frame.h"

class OpenGLPlayer {
 public:
  OpenGLPlayer() : window_(nullptr), renderer_(nullptr) {
    VLOG(2) << __func__;
  }
  ~OpenGLPlayer() {
    VLOG(2) << __func__;
    Destroy();
  }

  void Render(const std::shared_ptr<Frame> &frame) {
    VLOG(2) << "render frame " << frame->width() << "x" << frame->height();
    Init(frame);
    Draw(frame);
  }

 private:
  void Init(const std::shared_ptr<Frame> &frame) {
    if (window_ != nullptr) return;

    SDL_CreateWindowAndRenderer(frame->width(), frame->height(), 0, &window_,
        &renderer_);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // shader sources
    const char *vertex_shader_source = R"vs(
      attribute highp vec3 aPos;
      attribute vec2 aTexCoord;

      varying highp vec2 vTexCoord;

      void main() {
        gl_Position = vec4(aPos, 1.0);
        vTexCoord = aTexCoord;
      })vs";
    const char *fragment_shader_source = R"fs(
      precision highp float;
      varying lowp vec2 vTexCoord;

      uniform sampler2D yTex;
      uniform sampler2D uTex;
      uniform sampler2D vTex;

      // yuv420p to rgb888 matrix
      const mat4 YUV2RGB = mat4(
        1.1643828125,             0, 1.59602734375, -.87078515625,
        1.1643828125, -.39176171875,    -.81296875,     .52959375,
        1.1643828125,   2.017234375,             0,  -1.081390625,
                   0,             0,             0,             1
      );

      void main() {
        gl_FragColor = vec4(
          texture2D(yTex, vTexCoord).x,
          texture2D(uTex, vTexCoord).x,
          texture2D(vTex, vTexCoord).x,
          1
        ) * YUV2RGB;
      })fs";

    // build and compile our shader program
    shader_ = std::unique_ptr<Shader>(new Shader(
      vertex_shader_source, fragment_shader_source));

    // set up vertex data (and buffer(s)) and configure vertex attributes
    GLfloat vertices[] = {
      // positions         // texture coords
      -1.0f, -1.0f, 0.0f,  0.0f, 1.0f,  // bottom left
       1.0f, -1.0f, 0.0f,  1.0f, 1.0f,  // bottom right
      -1.0f,  1.0f, 0.0f,  0.0f, 0.0f,  // top left
       1.0f,  1.0f, 0.0f,  1.0f, 0.0f,  // top right
    };

    // create vertex array object
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    // create vertex buffer object
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // vertex attribute: position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
      reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    // vertex attribute: texture coord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
      reinterpret_cast<void*>(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // create textures
    texture_y_ = std::make_unique<Texture>();
    texture_u_ = std::make_unique<Texture>();
    texture_v_ = std::make_unique<Texture>();

    shader_->Use();
    texture_y_->Bind(0, shader_->ID, "yTex");
    texture_u_->Bind(1, shader_->ID, "uTex");
    texture_v_->Bind(2, shader_->ID, "vTex");
  }

  void Draw(const std::shared_ptr<Frame> &frame) {
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    {
      auto width = frame->width();
      auto height = frame->height();
      auto data = frame->data();

      auto len_y = width * height;
      auto len_u = (width >> 1) * (height >> 1);

      auto format = GL_LUMINANCE;
      texture_y_->Fill(width, height, data, format);
      texture_u_->Fill(width >> 1, height >> 1, data + len_y, format);
      texture_v_->Fill(width >> 1, height >> 1, data + len_y + len_u, format);
    }

    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    SDL_GL_SwapWindow(window_);
  }

  void Destroy() {
    if (window_ == nullptr) return;
    shader_->Delete();
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
    texture_y_->Delete();
    texture_u_->Delete();
    texture_v_->Delete();
    if (renderer_) {
      SDL_DestroyRenderer(renderer_);
      renderer_ = nullptr;
    }
    if (window_) {
      SDL_DestroyWindow(window_);
      window_ = nullptr;
    }
  }

  SDL_Window *window_;
  SDL_Renderer *renderer_;

  std::unique_ptr<Shader> shader_;
  GLuint vao_;
  GLuint vbo_;

  std::unique_ptr<Texture> texture_y_;
  std::unique_ptr<Texture> texture_u_;
  std::unique_ptr<Texture> texture_v_;
};
