#!/bin/bash

# Скрипт для сборки сервера

set -e

echo "╔════════════════════════════════════════╗"
echo "║   Minecraft Server Builder v1.0        ║"
echo "╚════════════════════════════════════════╝"
echo ""

# Проверяем наличие gcc
if ! command -v gcc &> /dev/null; then
    echo "[ERROR] gcc не найден. Пожалуйста, установите build-essential или GCC."
    exit 1
fi

# Проверяем наличие make
if ! command -v make &> /dev/null; then
    echo "[ERROR] make не найден. Пожалуйста, установите make."
    exit 1
fi

echo "[INFO] Версия GCC: $(gcc --version | head -n1)"
echo "[INFO] Версия Make: $(make --version | head -n1)"
echo ""

# Создаём директории
echo "[BUILD] Создание директорий..."
mkdir -p build world

# Сборка
echo "[BUILD] Сборка проекта..."
make clean
make -j$(nproc)

echo ""
echo "╔════════════════════════════════════════╗"
echo "║        СБОРКА ЗАВЕРШЕНА УСПЕШНО        ║"
echo "╚════════════════════════════════════════╝"
echo ""
echo "Для запуска выполните:"
echo "  ./build/server"
echo ""
echo "Или используйте:"
echo "  make run"
echo ""
