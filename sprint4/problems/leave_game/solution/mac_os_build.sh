#!/bin/bash

# Проверяем, существует ли папка build
if [ -d "build" ]; then
    echo "Папка build уже существует. Входим в неё..."
else
    echo "Папка build не найдена. Создаём и входим..."
    mkdir build
fi

cd build

# Устанавливаем зависимости через Conan
conan install .. --build=missing --profile=gcc-profile -o boost:without_fiber=True

# Конфигурируем CMake
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=/opt/homebrew/bin/gcc-14 -DCMAKE_CXX_COMPILER=/opt/homebrew/bin/g++-14 ..

# Собираем проект
cmake --build .

