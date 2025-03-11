#!/bin/bash

# Определяем режим сборки (по умолчанию debug)
BUILD_TYPE=${1:-debug}

# Выбираем папку сборки
if [[ "$BUILD_TYPE" == "release" ]]; then
    BUILD_DIR="release-build"
    CMAKE_BUILD_TYPE="Release"
else
    BUILD_DIR="build"
    CMAKE_BUILD_TYPE="Debug"
fi

echo "Building in $CMAKE_BUILD_TYPE mode..."

# Создаём папку сборки, если её нет
if [[ ! -d "$BUILD_DIR" ]]; then
    echo "Creating build directory: $BUILD_DIR"
    mkdir -p "$BUILD_DIR"
fi

# Запускаем CMake, если CMakeLists.txt ещё не был сгенерирован
if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
    cmake -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -B "$BUILD_DIR"
fi

# Запускаем сборку
cmake --build "$BUILD_DIR" -j$(nproc)

# Проверяем успешность сборки
if [[ ! -f "$BUILD_DIR/game_server" ]]; then
    echo "Error: game_server not found in $BUILD_DIR. Build failed?"
    exit 1
fi

echo "Build complete."

