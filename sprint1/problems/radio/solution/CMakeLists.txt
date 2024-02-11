cmake_minimum_required(VERSION 3.11)

project(Radio CXX)
set(CMAKE_CXX_STANDARD 20)

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Boost 1.78.0 REQUIRED)
find_package(Threads REQUIRED)

add_executable(radio src/main.cpp src/audio.h)
target_link_libraries(radio PRIVATE Threads::Threads ${Boost_Libraries})
