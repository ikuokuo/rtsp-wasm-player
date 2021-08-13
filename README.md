# RTSP Wasm Player

```txt
# RTSP WebSocket Proxy
RTSP/Webcam/File > FFmpeg open > Packets > WebSocket

# WS Wasm Player
WebSocket > Packets > Wasm FFmpeg decode to YUV > WebGL display
                                                > Wasm OpenGL display

# WS Local Player
WebSocket > Packets > FFmpeg decode to YUV > OpenGL display

# RTSP Local Player
RTSP/Webcam/File > FFmpeg open and decode to BGR/YUV > OpenCV/OpenGL display
```

## Prerequisites

- Ubuntu 18.04

Install develop tools:

```bash
sudo apt install -y build-essential git wget yasm

# cmake: https://cmake.org/download/
export PATH=./cmake-3.21.1-linux-x86_64/bin:$PATH
```

Prepare third-party dependencies:

```bash
git clone https://github.com/ikuokuo/rtsp-wasm-player.git
cd rtsp-wasm-player
export MY_ROOT=`pwd`

# boost: https://www.boost.org/
#  Beast: https://github.com/boostorg/beast
wget -p $MY_ROOT/3rdparty/ https://boostorg.jfrog.io/artifactory/main/release/1.76.0/source/boost_1_76_0.tar.bz2
cd $MY_ROOT/3rdparty/
tar --bzip2 -xf boost_1_76_0.tar.bz2
cd $MY_ROOT/3rdparty/boost_1_76_0/
./bootstrap.sh --prefix=$MY_ROOT/3rdparty/boost-1.76 --with-libraries=coroutine
./b2 install
ln -s boost-1.76 $MY_ROOT/3rdparty/boost

# ffmpeg: https://ffmpeg.org/
git clone --depth 1 -b n4.4 https://git.ffmpeg.org/ffmpeg.git $MY_ROOT/3rdparty/source/ffmpeg
cd $MY_ROOT/3rdparty/source/ffmpeg
./configure --prefix=$MY_ROOT/3rdparty/ffmpeg-4.4 \
--enable-gpl --enable-version3 \
--disable-programs --disable-doc --disable-everything \
--enable-decoder=h264 --enable-parser=h264 \
--enable-decoder=hevc --enable-parser=hevc \
--enable-hwaccel=h264_nvdec --enable-hwaccel=hevc_nvdec \
--enable-demuxer=rtsp \
--enable-demuxer=rawvideo --enable-decoder=rawvideo --enable-indev=v4l2 \
--enable-protocol=file \
--enable-bsf=h264_mp4toannexb --enable-bsf=hevc_mp4toannexb --enable-bsf=null
make -j`nproc`
make install
ln -s ffmpeg-4.4 $MY_ROOT/3rdparty/ffmpeg

# glog: https://github.com/google/glog
git clone --depth 1 -b v0.5.0 https://github.com/google/glog.git $MY_ROOT/3rdparty/source/glog
cd $MY_ROOT/3rdparty/source/glog
mkdir _build; cd _build
cmake -DCMAKE_INSTALL_PREFIX=$MY_ROOT/3rdparty/glog-0.5 ..
cmake --build . --target install --config Release -- -j`nproc`
ln -s glog-0.5 $MY_ROOT/3rdparty/glog

# glew, glfw3
sudo apt install -y libglew-dev libglfw3-dev
# glm
git clone --depth 1 -b 0.9.9.8 https://github.com/g-truc/glm.git $MY_ROOT/3rdparty/glm

# yaml-cpp: https://github.com/jbeder/yaml-cpp
git clone --depth 1 -b yaml-cpp-0.7.0 https://github.com/jbeder/yaml-cpp.git $MY_ROOT/3rdparty/source/yaml-cpp
cd $MY_ROOT/3rdparty/source/yaml-cpp
mkdir _build; cd _build
cmake -DCMAKE_INSTALL_PREFIX=$MY_ROOT/3rdparty/yaml-cpp-0.7.0 -DYAML_CPP_BUILD_TESTS=OFF ..
cmake --build . --target install --config Release -- -j`nproc`
ln -s yaml-cpp-0.7.0 $MY_ROOT/3rdparty/yaml-cpp
```

<!--
sudo apt install -y ffmpeg libboost-all-dev libgoogle-glog-dev
-->

## Modules

### RTSP WebSocket Proxy

```txt
RTSP/Webcam/File > FFmpeg open > Packets > WebSocket
```

Build and run:

```bash
cd rtsp-ws-proxy

# build
make

# run
./_output/bin/rtsp-ws-proxy ./config.yaml
```

### WS Wasm Player

```txt
WebSocket > Packets > Wasm FFmpeg decode to YUV > WebGL display
                                                > Wasm OpenGL display
```

### WS Local Player

```txt
WebSocket > Packets > FFmpeg decode to YUV > OpenGL display
```

### RTSP Local Player

```txt
RTSP/Webcam/File > FFmpeg open and decode to BGR/YUV > OpenCV/OpenGL display
```

Build and run:

```bash
cd rtsp-local-player

# if wanna enable build with opencv
# export OpenCV_DIR=~/opencv-4/lib/cmake/opencv4

# build
make

# run
./_output/bin/rtsp-local-player_ogl ./cfg_rtsp.yaml
#  or
./_output/bin/rtsp-local-player_ocv ./cfg_webcam.yaml
```
