cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

get_property(importTargets DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY IMPORTED_TARGETS)
message(STATUS "${importTargets}") 

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(game_server
  src/main.cpp
  src/http_server.cpp
  src/http_server.h
  src/sdk.h
  src/model.h
  src/model.cpp
  src/tagged.h
  src/boost_json.cpp
  src/json_loader.h
  src/json_loader.cpp
  src/request_handler.cpp
  src/request_handler.h
  src/url_parser.h
  src/url_parser.cpp
  src/util.cpp
  src/util.h
  src/log.cpp
  src/log.cpp
)

target_include_directories(game_server PRIVATE CONAN_PKG::boost)
target_link_libraries(game_server CONAN_PKG::boost) 