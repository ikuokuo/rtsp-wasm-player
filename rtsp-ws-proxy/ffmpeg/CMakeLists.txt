## encode_video
#  https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/encode_video.c

add_executable(encode_video encode_video.c)
target_link_libraries(encode_video PkgConfig::ffmpeg)

## stream_encode_video

add_executable(stream_encode_video
  ${MY_ROOT}/common/media/stream_def.cc
  ../stream_video_encoder.cc
  stream_encode_video.cc
)
target_link_libraries(stream_encode_video PkgConfig::ffmpeg glog::glog yaml-cpp)

# install

install(TARGETS encode_video stream_encode_video
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
