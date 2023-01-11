cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 11)

# Safie APIのサンプルプログラム
project(streaming-download CXX)

# pkg-configによりFFmpegおよびOpenCVを探索
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavformat>=58 libavcodec>=58 libavutil>=56)

# ターゲットの設定
add_executable(streaming-download streaming-download.cpp)
target_include_directories(streaming-download PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_libraries(streaming-download ${FFMPEG_LIBRARIES})
