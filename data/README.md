# Data

```bash
sudo apt install -y ffmpeg
```

## test.mp4

Download: https://archive.org/download/archive-video-files/test.mp4

```bash
$ ffprobe -hide_banner test.mp4
Input #0, mov,mp4,m4a,3gp,3g2,mj2, from 'test.mp4':
  Metadata:
    major_brand     : isom
    minor_version   : 512
    compatible_brands: isomiso2avc1mp41
    creation_time   : 1970-01-01T00:00:00.000000Z
    encoder         : Lavf53.24.2
  Duration: 00:00:13.50, start: 0.000000, bitrate: 1248 kb/s
    Stream #0:0(und): Video: h264 (Main) (avc1 / 0x31637661), yuv420p, 1280x720 [SAR 1:1 DAR 16:9], 862 kb/s, 25 fps, 25 tbr, 12800 tbn, 50 tbc (default)
    Metadata:
      creation_time   : 1970-01-01T00:00:00.000000Z
      handler_name    : VideoHandler
    Stream #0:1(und): Audio: aac (LC) (mp4a / 0x6134706D), 48000 Hz, 5.1, fltp, 381 kb/s (default)
    Metadata:
      creation_time   : 1970-01-01T00:00:00.000000Z
      handler_name    : SoundHandler
```

Convert to `h265/hevc`:

```bash
ffmpeg -i test.mp4 -c:v hevc -c:a copy test_h265.mp4
```

<!--
acodec = -codec:a = -c:a
vcodec = -codec:v = -c:v
-->
