@echo off
cd /d %~dp0

REM Set paths to the Emscripten tools
set EM_PATH=F:\Repositories\emsdk
set EM_BIN_PATH=F:\Repositories\emsdk\upstream\bin
set PATH=%EM_PATH%;%EM_BIN_PATH%;%PATH%

REM Activate Emscripten
call %EM_PATH%\emsdk.bat activate latest
call %EM_PATH%\emsdk_env.bat

REM Compile your C++ code with Emscripten
em++ F:\Repositories\C++RogueLike\C++RogueLike\main_C++RogueLike.cpp -o F:\Repositories\C++RogueLike\emscripten\index.html

REM If the compile succeeds, run a local web server to serve the compiled output
if %errorlevel%==0 (
    echo Compilation succeeded. Starting local web server...
    python -m http.server 8080
) else (
    echo Compilation failed.
    pause
)
