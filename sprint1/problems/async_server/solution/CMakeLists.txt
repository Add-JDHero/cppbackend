cmake_minimum_required(VERSION 3.11)

project(HelloAsync CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_COLOR_DIAGNOSTICS ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(hello_async src/main.cpp src/http_server.cpp src/http_server.h src/sdk.h)
target_link_libraries(hello_async PRIVATE Threads::Threads ${Boost_Libraries})
