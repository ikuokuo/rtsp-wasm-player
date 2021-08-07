#include <glog/logging.h>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "common/media/stream.h"

int main(int argc, char const *argv[]) {
  (void)argc;
  FLAGS_logtostderr = true;
  FLAGS_colorlogtostderr = true;
  FLAGS_minloglevel = google::INFO;
  FLAGS_v = 2;
  google::InitGoogleLogging(argv[0]);

  StreamOptions options{};
  /*{
    options.method = STREAM_METHOD_WEBCAM;
    options.input_url = "/dev/video0";

    // v4l2-ctl --list-devices
    // v4l2-ctl -d 0 --list-formats-ext
    // ffmpeg -hide_banner -f v4l2 -list_formats all -i /dev/video0
    options.input_format = "v4l2";
    options.width = 640;
    options.height = 480;
    options.framerate = 20;
    options.pixel_format = AV_PIX_FMT_YUYV422;

    options.sws_enable = true;
    options.sws_dst_pix_fmt = AV_PIX_FMT_BGR24;
  }*/
  {
    /*
    vlc file:///$HOME/Videos/test.mp4 --loop \
    --sout '#gather:transcode{vcodec=h264}:rtp{sdp=rtsp://:8554/test}' \
    --network-caching=1500 --sout-all --sout-keep
    */
    options.method = STREAM_METHOD_NETWORK;
    options.input_url = "rtsp://127.0.0.1:8554/test";

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
