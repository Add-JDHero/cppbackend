cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

# get_property(importTargets DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY IMPORTED_TARGETS)
# message(STATUS "${importTargets}") 

add_executable(game_server
  src/main.cpp
  src/http_server.cpp
  src/http_server.h
  src/sdk.h
  src/model.h
  src/model.cpp
  src/application.h
  src/application.cpp
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
  src/log.h
  src/router.h
  src/router.cpp
  src/type_declarations.h
  src/handlers.h
  src/handlers.cpp
  src/util_tests.h
  src/util_tests.cpp
  src/ticker.h
  src/ticker.cpp
  src/command_line_parser.h
  src/player.h
  src/player.cpp
  src/extra_data.h
  src/extra_data.cpp
  src/loot_generator.h
  src/loot_generator.cpp
  src/geom.h
  src/collision_detector.h
  src/collision_detector.cpp
)

target_link_libraries(game_server CONAN_PKG::boost) 
