cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 11)

# Safie APIのサンプルプログラム
project(mediafile-download CXX)

# pkg-configによりlibcurl, cJSONを探索
find_package(PkgConfig REQUIRED)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(CJSON REQUIRED libcjson)

# ターゲットの設定
add_executable(mediafile-download mediafile-download.cpp)
target_include_directories(mediafile-download PRIVATE ${CURL_INCLUDE_DIRS} ${CJSON_INCLUDE_DIRS})
target_link_libraries(mediafile-download ${CURL_LIBRARIES} ${CJSON_LIBRARIES})
