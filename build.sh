#!/usr/bin/env bash

set -e

echo "Building Pixel releases..."

# Clean
rm -rf pixel-release
rm -f pixel-release.zip

mkdir -p pixel-release/linux/lib
mkdir -p pixel-release/windows/lib

# Compiler settings
COMMON_FLAGS="-std=c++17 -O3 -s -I."

SOURCES="
main.cpp
Lexer.cpp
Parser.cpp
TypeChecker.cpp
Interpreter.cpp
"

# lin build
echo "Building Linux version..."

g++ $COMMON_FLAGS \
    -o pixel-release/linux/pixel \
    $SOURCES

echo "Linux build complete"

# win build
echo "Building Windows version..."

if ! command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1; then
    echo "ERROR: mingw-w64 not installed"
    echo "Install with:"
    echo "  Ubuntu/Debian: sudo apt install mingw-w64"
    exit 1
fi

x86_64-w64-mingw32-g++ $COMMON_FLAGS \
    -o pixel-release/windows/pixel.exe \
    $SOURCES

echo "Windows build complete"


# copy lib
echo "Copying libraries..."

if [ -d "lib" ]; then
    cp -r lib/ pixel-release/linux/
    cp -r lib/ pixel-release/windows/
fi


# zip
echo "Creating archive..."

zip -r pixel-release.zip pixel-release >/dev/null


echo ""
echo "Done!"
echo "Created: pixel-release.zip"
