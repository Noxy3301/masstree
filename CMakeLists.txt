cmake_minimum_required(VERSION 3.13)

set(ENV{CXX} g++)
project(masstree)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

file(GLOB MASSTREE_SOURCES src/*.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_executable(masstree.exe ${MASSTREE_SOURCES})

target_compile_options(masstree.exe PUBLIC -O0 -g -std=c++17 -m64)

# GoogleTestのダウンロードとビルド
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

# テスト用の実行ファイルの設定
enable_testing()

file(GLOB_RECURSE TEST_SOURCES test/*.cpp src/*.cpp)
# src/main.cpp を除外
list(REMOVE_ITEM TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
add_executable(tests ${TEST_SOURCES})

target_link_libraries(tests gtest_main)

# テストの自動検出（オプション）
include(GoogleTest)
gtest_discover_tests(tests)