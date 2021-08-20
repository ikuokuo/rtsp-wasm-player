#include <condition_variable>
#include <mutex>

#include "common/gl/glfw_frame.h"
#include "common/media/stream_thread.h"
#include "common/util/config.h"

int main(int argc, char const *argv[]) {
  config::InitGoogleLoggingFlags();
  google::InitGoogleLogging(argv[0]);

  if (argc < 2) {
    LOG(ERROR) << "Usage: <program> config.yaml";
    return 1;
  }

  LOG(INFO) << "Load config: " << argv[1];
  YAML::Node node;
  StreamOptions options{};
  try {
    node = YAML::LoadFile(argv[1]);
    config::InitGoogleLoggingFlags(node["log"]);
    options = node["stream"].as<StreamOptions>();
  } catch (const std::exception &e) {
    LOG(ERROR) << " parse options fail, " << e.what();
    return 1;
  }

  if (options.video.sws_enable != true ||
      options.video.sws_dst_pix_fmt != AV_PIX_FMT_YUV420P) {
    LOG(WARNING) << " sws change to enable and yuv420p (for opengl display)";
    options.video.sws_enable = true;
    options.video.sws_dst_pix_fmt = AV_PIX_FMT_YUV420P;
  }

  StreamThread stream;
  int stream_get_frequency = 20;

  GlfwFrame ui;

  bool frame_first_got{false};
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
    &ui, &frame_first_got, &frame_first, &mutex_frame_first, &cond_frame_first
  ](const std::shared_ptr<StreamEvent> &e) {
    if (e->id == STREAM_EVENT_GET_FRAME) {
      auto event = std::dynamic_pointer_cast<StreamFrameEvent>(e);
      auto frame = event->frame;
      if (frame == nullptr) return;

      ui.Update(frame);

      if (!frame_first_got) {
        // notify get first frame
        std::unique_lock<std::mutex> lock(mutex_frame_first);
        frame_first_got = true;
        frame_first = frame;
        cond_frame_first.notify_one();
      }
    } else if (e->id == STREAM_EVENT_LOOP) {
      LOG(WARNING) << "Stream loop ...";
      // frame is invalid, wait update
      ui.Update(nullptr);
    } else if (e->id == STREAM_EVENT_ERROR) {
      auto event = std::dynamic_pointer_cast<StreamErrorEvent>(e);
      LOG(ERROR) << event->error.what();
    }
  });
  stream.Start(options, stream_get_frequency);

  {
    // wait get first frame until timeout
    std::unique_lock<std::mutex> lock(mutex_frame_first);
    cond_frame_first.wait_for(lock,
      std::chrono::seconds(frame_first_wait_secs),
      [&frame_first_got]() { return frame_first_got; });
  }
  int ret = 0;
  if (frame_first_got) {
    LOG(INFO) << "Get first frame: " << frame_first->width << "x"
      << frame_first->height;
    // show ui after get first frame
    GlfwInitParams params{
      frame_first->width, frame_first->height, options.input_url
    };
    frame_first = nullptr;
    ret =  ui.Run(params);
  } else {
    LOG(ERROR) << "Get first frame timeout, > " << frame_first_wait_secs
      << " s";
  }

  stream.Stop();
  return ret;
}
