#include <fstream>

#include "common/util/config.h"

#include "../stream_video_encoder.h"

int main(int argc, char const *argv[]) {
  config::InitLogging(argv[0]);

  const char *filename, *codec_name;
  filename = argc >= 2 ? argv[1] : "test.h264";
  codec_name = argc >= 3 ? argv[2] : "h264_nvenc";

  StreamVideoEncodeOptions options{};
  options.codec_name = codec_name;
  options.codec_bit_rate = 400000;
  options.codec_width = 352;
  options.codec_height = 288;
  options.codec_framerate = 25;
  options.codec_gop_size = 10;
  options.codec_max_b_frames = 1;
  options.codec_pix_fmt = AV_PIX_FMT_YUV420P;
  options.open_options = {
    {"preset", "slow"},
  };

  StreamVideoEncoder encoder{options};

  AVFrame *frame = av_frame_alloc();
  if (!frame) {
    LOG(ERROR) << "Could not allocate video frame";
    return 1;
  }
  frame->width  = options.codec_width;
  frame->height = options.codec_height;
  frame->format = options.codec_pix_fmt;

  int ret = av_frame_get_buffer(frame, 0);
  if (ret < 0) {
    LOG(ERROR) << "Could not allocate the video frame data";
    return 1;
  }

  std::fstream f(filename, std::fstream::out | std::fstream::binary);
  auto encode_recv = [&f](AVPacket *pkt) {
    LOG(INFO) << "Write packet " << pkt->pts << ", size=" << pkt->size;
    f.write(reinterpret_cast<char *>(pkt->data), pkt->size);
  };

  int i, x, y;
  /* encode 1 second of video */
  for (i = 0; i < 25; i++) {
    ret = av_frame_make_writable(frame);
    if (ret < 0) return 1;

    /* Y */
    for (y = 0; y < frame->height; y++) {
      for (x = 0; x < frame->width; x++) {
        frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
      }
    }
    /* Cb and Cr */
    for (y = 0; y < frame->height/2; y++) {
      for (x = 0; x < frame->width/2; x++) {
        frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
        frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
      }
    }

    frame->pts = i;

    LOG(INFO) << "Encode frame " << i;
    encoder.Encode(frame, encode_recv);
  }

  LOG(INFO) << "Encode flush";
  encoder.Encode(NULL, encode_recv);

  auto codec_id = encoder.GetCodecContext()->codec_id;
  if (codec_id == AV_CODEC_ID_MPEG1VIDEO ||
      codec_id == AV_CODEC_ID_MPEG2VIDEO) {
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };
    f.write(reinterpret_cast<char *>(endcode), sizeof(endcode));
  }
  f.flush();
  f.close();
  LOG(INFO) << "Write file " << filename;

  return 0;
}
