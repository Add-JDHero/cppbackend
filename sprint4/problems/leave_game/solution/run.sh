#!/bin/bash

# Определяем режим (по умолчанию debug)
BUILD_TYPE=${1:-debug}

# Выбираем папку сборки
if [[ "$BUILD_TYPE" == "release" ]]; then
    BUILD_DIR="release-build"
else
    BUILD_DIR="build"
fi

# Проверяем, есть ли уже собранный бинарник
EXECUTABLE="$BUILD_DIR/game_server"
if [[ ! -f "$EXECUTABLE" ]]; then
    echo "No compiled binary found in $BUILD_DIR. Running build..."
    ./build.sh "$BUILD_TYPE"  # Запускаем сборку перед запуском
fi

# Запуск сервера
echo "Starting game server in $BUILD_TYPE mode..."
"$EXECUTABLE" -c ./data/config.json -w ./static -t 10 -s ./src/server_state.txt

