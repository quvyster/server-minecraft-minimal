@echo off
REM Скрипт сборки для Windows (MSYS2 MINGW64)

echo.
echo ╔════════════════════════════════════════╗
echo ║   Minecraft Server Builder (Windows)   ║
echo ╚════════════════════════════════════════╝
echo.

REM Проверяем наличие MSYS2
where gcc >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] gcc не найден!
    echo.
    echo Пожалуйста, установите MSYS2 MINGW64:
    echo 1. Загрузите MSYS2 с https://www.msys2.org/
    echo 2. Установите зависимости:
    echo    pacman -Sy mingw-w64-x86_64-gcc mingw-w64-x86_64-make
    echo 3. Запустите "MSYS2 MINGW64" терминал
    echo 4. Перейдите в папку проекта и выполните build.bat
    pause
    exit /b 1
)

echo [INFO] Найден компилятор:
gcc --version | head -n 1

echo.
echo [BUILD] Создание директорий...
if not exist build mkdir build
if not exist world mkdir world

echo [BUILD] Сборка проекта...
make clean
make -j%NUMBER_OF_PROCESSORS%

if %errorlevel% equ 0 (
    echo.
    echo ╔════════════════════════════════════════╗
    echo ║        СБОРКА ЗАВЕРШЕНА УСПЕШНО        ║
    echo ╚════════════════════════════════════════╝
    echo.
    echo Для запуска выполните:
    echo   build\server.exe
    echo.
    echo Или используйте:
    echo   make run
    echo.
) else (
    echo.
    echo [ERROR] Ошибка при компиляции!
    echo.
)

pause
