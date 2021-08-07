cmake_minimum_required(VERSION 3.10)
project(rtsp-local-player VERSION 1.0.0 LANGUAGES C CXX CUDA)

set(MY_NAME ${PROJECT_NAME})
set(MY_ROOT ${PROJECT_SOURCE_DIR})
get_filename_component(MY_COMMON "${MY_ROOT}/../common" REALPATH)
get_filename_component(MY_3RDPARTY "${MY_ROOT}/../3rdparty" REALPATH)

set(MY_OUTPUT "${MY_ROOT}/_output")

# options

option(WITH_OPENCV      "With OpenCV"     ON)
option(WITH_OPENGL      "With OpenGL"     ON)

# macros

## msg_var(variableName)
macro(msg_var variableName)
  message(STATUS "${variableName}: ${${variableName}}")
endmacro()

msg_var(MY_ROOT)
msg_var(MY_COMMON)
msg_var(MY_3RDPARTY)

# cmake

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()
if(NOT CMAKE_DEBUG_POSTFIX)
  set(CMAKE_DEBUG_POSTFIX d)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
#set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)

## flags

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

# packages

LIST(APPEND CMAKE_MODULE_PATH "${MY_ROOT}/cmake/modules")

## threads

find_package(Threads REQUIRED)

## ffmpeg

set(ENV{PKG_CONFIG_PATH} "${MY_3RDPARTY}/ffmpeg/lib/pkgconfig")
find_package(PkgConfig REQUIRED)
pkg_check_modules(ffmpeg REQUIRED IMPORTED_TARGET
  libavcodec libavdevice libavformat libavutil libswscale
)

## glog

set(glog_DIR "${MY_3RDPARTY}/glog/lib/cmake")
find_package(glog REQUIRED)
msg_var(glog_VERSION)

## opencv

if(WITH_OPENCV)
  set(OpenCV_DIR "$ENV{HOME}/opencv-4/lib/cmake/opencv4")
  find_package(OpenCV REQUIRED)
endif()

# targets

include_directories(
  ${MY_COMMON}
  ${OpenCV_INCLUDE_DIRS}
)

file(GLOB MY_COMMON_SRCS "${MY_COMMON}/*.cc")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${MY_OUTPUT}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${MY_OUTPUT}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${MY_OUTPUT}/bin")
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
  string(TOUPPER ${CONFIG} CONFIG)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG} ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endforeach()

## ui with opencv

if(WITH_OPENCV)

add_executable(${MY_NAME}_ocv
  ${MY_COMMON_SRCS}
  main_ui_with_opencv.cc
)
target_link_libraries(${MY_NAME}_ocv
  PkgConfig::ffmpeg glog::glog ${OpenCV_LIBS}
)

endif()

# install

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
set(CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

set(CMAKE_MACOSX_RPATH 1)
#set(CMAKE_INSTALL_RPATH "${MY_CMAKE_RPATH}")

install(TARGETS ${MY_NAME}_ocv
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
