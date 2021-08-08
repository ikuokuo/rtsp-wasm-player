#include <condition_variable>
#include <memory>
#include <mutex>

#include <GL/glew.h>
#include <glog/logging.h>

#include "common/gl/glfw_base.h"
#include "common/gl/shader.h"
#include "common/gl/texture.h"
#include "common/media/stream_thread.h"
#include "common/util/options.h"

class Ui : public GlfwBase {
 public:
  Ui() {}
  ~Ui() = default;

  void Update(AVFrame *frame) {
    frame_ = frame;
  }

 protected:
  void OnInit() override {
    // initialize glew
    glewExperimental = true;  // needed in core profile
    if (glewInit() != GLEW_OK) {
      LOG(ERROR) << "Failed to initialize GLEW";
      return;
    }
    LOG(INFO) << "Renderer: " << glGetString(GL_RENDERER);
    LOG(INFO) << "OpenGL version supported " << glGetString(GL_VERSION);

    // shader sources
    const char *vertex_shader_source = R"vs(
      #version 330 core
      layout (location = 0) in vec3 aPos;
      layout (location = 1) in vec2 aTexCoord;

      out vec2 vTexCoord;

      void main() {
        gl_Position = vec4(aPos, 1.0);
        vTexCoord = aTexCoord;
      })vs";
    const char *fragment_shader_source = R"fs(
      #version 330 core
      in vec2 vTexCoord;

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
        // gl_FragColor = vec4(vTexCoord.x, vTexCoord.y, 0., 1.0f);
        gl_FragColor = vec4(
          texture(yTex, vTexCoord).x,
          texture(uTex, vTexCoord).x,
          texture(vTex, vTexCoord).x,
          1
        ) * YUV2RGB;
      })fs";

    // build and compile our shader program
    shader_ = std::unique_ptr<Shader>(new Shader(
      vertex_shader_source, fragment_shader_source));

    // set up vertex data (and buffer(s)) and configure vertex attributes
    //
    // vertex coords   vertex index   tex coords
    // -1,+1 - +1,+1   3 — 4          0,1 - 1,1
    //   |       |     | \ |           |     |
    // -1,-1 - +1,-1   1 - 2          0,0 - 1,0
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

  void OnDraw() override {
    if (frame_ != nullptr) {
      auto width = frame_->width;
      auto height = frame_->height;
      auto data = frame_->data[0];

      auto len_y = width * height;
      auto len_u = (width >> 1) * (height >> 1);

      texture_y_->Fill(width, height, data);
      texture_u_->Fill(width >> 1, height >> 1, data + len_y);
      texture_v_->Fill(width >> 1, height >> 1, data + len_y + len_u);
    }

    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }

  void OnDestroy() override {
    shader_->Delete();
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
  }

 private:
  std::unique_ptr<Shader> shader_;
  GLuint vao_;
  GLuint vbo_;

  std::unique_ptr<Texture> texture_y_;
  std::unique_ptr<Texture> texture_u_;
  std::unique_ptr<Texture> texture_v_;

  AVFrame *frame_ = nullptr;
};

int main(int argc, char const *argv[]) {
  (void)argc;
  FLAGS_logtostderr = true;
  FLAGS_colorlogtostderr = true;
  FLAGS_minloglevel = google::INFO;
  FLAGS_v = 2;
  google::InitGoogleLogging(argv[0]);

  if (argc < 2) {
    LOG(ERROR) << "Usage: <program> config.yaml";
    return 1;
  }

  LOG(INFO) << "Load config: " << argv[1];
  StreamOptions options{};
  auto node = YAML::LoadFile(argv[1]);
  if (!StreamOptionsParse(node, &options)) return 1;

  if (!options.sws_enable || options.sws_dst_pix_fmt != AV_PIX_FMT_YUV420P) {
    LOG(WARNING) << " sws change to enable and yuv420p (for opengl display)";
    options.sws_enable = true;
    options.sws_dst_pix_fmt = AV_PIX_FMT_YUV420P;
  }

  StreamThread stream;
  int stream_get_frequency = 20;

  Ui ui;

  AVFrame *frame_first = nullptr;
  std::mutex mutex_frame_first;
  std::condition_variable cond_frame_first;
  int frame_first_wait_secs = 5;

  if (node["ui_gl"]) {
    auto ui_gl = node["ui_gl"];
    if (ui_gl["stream_get_frequency"])
      stream_get_frequency = ui_gl["stream_get_frequency"].as<int>();
    if (ui_gl["frame_first_wait_secs"])
      frame_first_wait_secs = ui_gl["frame_first_wait_secs"].as<int>();
  }

  stream.SetEventCallback([
    &ui, &frame_first, &mutex_frame_first, &cond_frame_first
  ](std::shared_ptr<StreamEvent> e) {
    if (e->id == STREAM_EVENT_GET_FRAME) {
      auto event = std::dynamic_pointer_cast<StreamFrameEvent>(e);
      auto frame = event->frame;
      if (frame == nullptr) return;

      ui.Update(frame);

      if (frame_first == nullptr) {
        // notify get first frame
        std::unique_lock<std::mutex> lock(mutex_frame_first);
        frame_first = frame;
        cond_frame_first.notify_one();
      }
    }
  });
  stream.Start(options, stream_get_frequency);

  {
    // wait get first frame until timeout
    std::unique_lock<std::mutex> lock(mutex_frame_first);
    cond_frame_first.wait_for(lock,
      std::chrono::seconds(frame_first_wait_secs),
      [&frame_first]() { return frame_first != nullptr; });
  }
  int ret = 0;
  if (frame_first != nullptr) {
    LOG(INFO) << "Get first frame: " << frame_first->width << "x"
      << frame_first->height;
    // show ui after get first frame
    ret =  ui.Run({
      frame_first->width, frame_first->height, options.input_url});
  } else {
    LOG(ERROR) << "Get first frame timeout, > " << frame_first_wait_secs
      << " s";
  }

  stream.Stop();
  return ret;
}
