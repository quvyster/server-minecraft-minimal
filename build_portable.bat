@echo off
rem Простая обёртка для сборки переносимого server-portable.exe
rem 1) Попытка использовать MSYS2 MinGW64 (путь можно изменить)
setlocal
set MSYS_MINGW=C:\msys64\mingw64\bin
if exist "%MSYS_MINGW%\gcc.exe" (
    echo Found MSYS2 MinGW at %MSYS_MINGW%
    set PATH=%MSYS_MINGW%;C:\msys64\usr\bin;%PATH%
) else (
    echo WARNING: MSYS2 MinGW not found at %MSYS_MINGW%
    echo Please install MSYS2 (https://www.msys2.org/) and install mingw-w64 toolchain.
)

rem Stop any running server processes we know by name
echo Stopping existing server processes (if any)...
taskkill /IM server-portable.exe /F >nul 2>&1
taskkill /IM server.exe /F >nul 2>&1

echo Running make portable...
make portable
if %ERRORLEVEL% EQU 0 (
    echo Portable build succeeded: build\server-portable.exe
    exit /b 0
)

echo Portable build failed. Attempting fallback: copy runtime DLLs next to the dynamic build.
if not exist build mkdir build
copy /Y "%MSYS_MINGW%\libwinpthread-1.dll" build\ >nul 2>&1
copy /Y "%MSYS_MINGW%\libgcc_s_seh-1.dll" build\ >nul 2>&1
copy /Y "%MSYS_MINGW%\libstdc++-6.dll" build\ >nul 2>&1

if exist build\server.exe (
    echo Copied DLLs to build\ — try running build\server.exe now.
    exit /b 0
) else (
    echo Fallback failed: build\server.exe not found. Check make output above.
    exit /b 1
)
