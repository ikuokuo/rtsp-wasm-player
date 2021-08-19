cmake_minimum_required(VERSION 3.10)
project(rtsp-ws-proxy VERSION 1.0.0 LANGUAGES C CXX)

set(MY_NAME ${PROJECT_NAME})
set(MY_CURR ${PROJECT_SOURCE_DIR})
get_filename_component(MY_ROOT "${MY_CURR}/.." REALPATH)
get_filename_component(MY_3RDPARTY "${MY_ROOT}/3rdparty" REALPATH)

set(MY_OUTPUT "${MY_CURR}/_output")
if(DEFINED ENV{MY_OUTPUT})
  set(MY_OUTPUT $ENV{MY_OUTPUT})
endif()

include(${MY_ROOT}/cmake/global.cmake)

msg_var(MY_CURR)
msg_var(MY_ROOT)
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

## boost

set(Boost_USE_DEBUG_LIBS OFF)
set(Boost_USE_RELEASE_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME OFF)
set(Boost_NO_SYSTEM_PATHS TRUE)
if(Boost_NO_SYSTEM_PATHS)
  set(BOOST_ROOT "${MY_3RDPARTY}/boost")
  set(BOOST_INCLUDE_DIRS "${BOOST_ROOT}/include")
  set(BOOST_LIBRARY_DIRS "${BOOST_ROOT}/lib")
endif()
find_package(Boost 1.76 REQUIRED context coroutine)

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

## yaml-cpp

set(yaml-cpp_DIR "${MY_3RDPARTY}/yaml-cpp/share/cmake/yaml-cpp")
find_package(yaml-cpp REQUIRED)
msg_var(yaml-cpp_VERSION)

# targets

include_directories(
  ${MY_ROOT}
  ${Boost_INCLUDE_DIRS}
  ${MY_3RDPARTY}/json/single_include
)

file(GLOB MY_COMMON_MEDIA_SRCS "${MY_ROOT}/common/media/*.cc")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${MY_OUTPUT}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${MY_OUTPUT}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${MY_OUTPUT}/bin")
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
  string(TOUPPER ${CONFIG} CONFIG)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG} ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endforeach()

## rtsp-ws-proxy

add_executable(rtsp-ws-proxy
  ${MY_COMMON_MEDIA_SRCS}
  main.cc
  ws_ext.cc
  ws_server.cc
  ws_stream_server.cc
  stream_handler.cc
)
target_link_libraries(rtsp-ws-proxy
  ${Boost_LIBRARIES} PkgConfig::ffmpeg glog::glog yaml-cpp
)

# install

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
set(CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

set(CMAKE_MACOSX_RPATH 1)
#set(CMAKE_INSTALL_RPATH "${MY_CMAKE_RPATH}")

install(TARGETS rtsp-ws-proxy
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
