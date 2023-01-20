cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 11)

# Safie APIのサンプルプログラム
project(get-image-set-flag CXX)

# pkg-configによりlibcurlを探索
find_package(PkgConfig REQUIRED)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(CJSON REQUIRED libcjson)

# ターゲットの設定
add_executable(get-image-set-flag get-image-set-flag.cpp)
target_include_directories(get-image-set-flag PRIVATE ${CURL_INCLUDE_DIRS} ${CJSON_INCLUDE_DIRS})
target_link_libraries(get-image-set-flag ${CURL_LIBRARIES} ${CJSON_LIBRARIES})
