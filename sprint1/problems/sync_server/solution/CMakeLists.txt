cmake_minimum_required(VERSION 3.11)

project(Hello CXX)
set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  # Boost найден, добавляем к каталогам заголовочных файлов проекта путь к
  # заголовочным файлам Boost
  include_directories(${Boost_INCLUDE_DIRS})
endif()

find_package(Boost 1.78.0 REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(hello src/main.cpp)
target_link_libraries(hello PRIVATE Threads::Threads ${Boost_Libraries})
