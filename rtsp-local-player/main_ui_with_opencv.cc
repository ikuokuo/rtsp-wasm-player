#include <glog/logging.h>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#define UTIL_CONFIG_STREAM
#include "common/media/stream.h"
#include "common/util/config.h"
#include "common/util/rate.h"

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
      options.video.sws_dst_pix_fmt != AV_PIX_FMT_BGR24) {
    LOG(WARNING) << " sws change to enable and bgr24 (for opencv display)";
    options.video.sws_enable = true;
    options.video.sws_dst_pix_fmt = AV_PIX_FMT_BGR24;
  }

  auto win_name = options.input_url;
  cv::namedWindow(win_name);

  int stream_get_frequency = 100;
  if (node["ui_cv"]) {
    auto ui_cv = node["ui_cv"];
    if (ui_cv["stream_get_frequency"])
      stream_get_frequency = ui_cv["stream_get_frequency"].as<int>();
  }

  try {
    Stream stream;
    stream.Open(options);

    Rate rate(stream_get_frequency);
    while (1) {
      auto frame = stream.GetFrameVideo();
      if (frame != nullptr) {
        cv::Mat image(frame->height, frame->width, CV_8UC3,
          frame->data[0], frame->linesize[0]);
        cv::imshow(win_name, image);
      }
      char key = static_cast<char>(cv::waitKey(1));
      if (key == 27 || key == 'q' || key == 'Q') {  // ESC/Q
        break;
      }
      rate.Sleep();
    }

    stream.Close();
  } catch (const StreamError &err) {
    LOG(ERROR) << err.what();
  }

  cv::destroyAllWindows();
  return 0;
}
