cmake_minimum_required(VERSION 3.11)

project(restaurant CXX)
set(CMAKE_CXX_STANDARD 20)

find_package(Boost 1.78.0 REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(restaurant src/main.cpp)
target_link_libraries(restaurant PRIVATE Threads::Threads ${Boost_Libraries})
