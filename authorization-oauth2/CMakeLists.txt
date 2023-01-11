cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 11)

# Safie APIのサンプルプログラム
project(authorization-oauth2 CXX)

# pkg-configによりlibcurlを探索
find_package(PkgConfig REQUIRED)
pkg_check_modules(CURL REQUIRED libcurl)

# ターゲットの設定
add_executable(list-devices list-devices.cpp)
target_include_directories(list-devices PRIVATE ${CURL_INCLUDE_DIRS})
target_link_libraries(list-devices ${CURL_LIBRARIES})
