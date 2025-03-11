cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

# Указываем путь к заголовкам
# include_directories(/usr/include/pqxx)

# # Ищем библиотеку
# find_library(PQXX_LIB pqxx REQUIRED HINTS /usr/lib/aarch64-linux-gnu)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)
find_library(PQ_LIB pq REQUIRED)

# Добавляем найденные пути к заголовкам
include_directories(${PQXX_INCLUDE_DIRS})
link_directories(${PQXX_LIBRARY_DIRS})

# Добавляем библиотеку к линковке
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
  src/model_serialization.h
  src/model_serialization.cpp
  src/database_manager.h
  src/database_manager.cpp
)

target_link_libraries(game_server 
  CONAN_PKG::boost
  # ${PQXX_LIB}
  ${PQXX_LIBRARIES}
  ${PQ_LIB}
) 
