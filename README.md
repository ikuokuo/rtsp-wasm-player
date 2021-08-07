# RTSP Wasm Player

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
--enable-demuxer=rawvideo --enable-decoder=rawvideo --enable-indev=v4l2
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
```

<!--
sudo apt install -y ffmpeg libboost-all-dev libgoogle-glog-dev
-->

## Modules

### RTSP Local Player

```txt
RTSP > FFmpeg open and decode to YUV > OpenGL display
```

Build:

```bash
cd rtsp-local-player
make install
```
