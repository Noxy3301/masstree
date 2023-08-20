cmake_minimum_required(VERSION 3.13)

set(ENV{CXX} g++)
project(masstree)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

file(GLOB MASSTREE_SOURCES
    "src/main.cpp"
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_executable(masstree.exe ${MASSTREE_SOURCES})

target_compile_options(masstree.exe PUBLIC -O0 -g -std=c++17 -m64)