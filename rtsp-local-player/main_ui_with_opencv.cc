#include <glog/logging.h>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "common/media/stream.h"
#include "common/util/options.h"

int main(int argc, char const *argv[]) {
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
  if (!StreamOptionsParse(argv[1], &options)) return 1;

  if (!options.sws_enable || options.sws_dst_pix_fmt != AV_PIX_FMT_BGR24) {
    LOG(WARNING) << " sws change to enable and bgr24 (for opencv display)";
    options.sws_enable = true;
    options.sws_dst_pix_fmt = AV_PIX_FMT_BGR24;
  }

  auto win_name = options.input_url;
  cv::namedWindow(win_name);

  try {
    Stream stream;
    stream.Open(options);

    while (1) {
      auto frame = stream.GetFrameVideo();
      if (frame != nullptr) {
        cv::Mat image(frame->height, frame->width, CV_8UC3,
          frame->data[0], frame->linesize[0]);
        cv::imshow(win_name, image);
      }
      char key = static_cast<char>(cv::waitKey(10));
      if (key == 27 || key == 'q' || key == 'Q') {  // ESC/Q
        break;
      }
    }

    stream.Close();
  } catch (const StreamError &err) {
    LOG(ERROR) << err.what();
  }

  cv::destroyAllWindows();
  return 0;
}
