@echo off
echo Building Fluid Swirl Plugin for DaVinci Resolve...

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake (adjust generator as needed)
cmake .. -G "Visual Studio 17 2022" -A x64

REM Build the plugin
cmake --build . --config Release

echo.
echo Build complete! Plugin bundle created at: build\FluidSwirl.ofx.bundle
echo.
echo To install:
echo 1. Copy the entire FluidSwirl.ofx.bundle folder to:
echo    C:\Program Files\Common Files\OFX\Plugins\
echo 2. Restart DaVinci Resolve
echo 3. Find "Fluid Swirl" in the Effects Library under Filter category
echo.
pause