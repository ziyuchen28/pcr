@echo off
setlocal

:: -G Ninja forces it to act like Linux (Single-Config) and generates compile_commands.json
:: -DCMAKE_BUILD_TYPE=Debug now works perfectly on Windows!
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
if %errorlevel% neq 0 exit /b %errorlevel%

:: -j works flawlessly with Ninja to use all CPU cores
cmake --build build -j
if %errorlevel% neq 0 exit /b %errorlevel%

echo Build Complete!
