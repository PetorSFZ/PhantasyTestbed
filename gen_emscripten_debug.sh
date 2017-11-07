#!/bin/bash
rm -rf build_emscripten_debug
mkdir build_emscripten_debug
cd build_emscripten_debug
cmake .. -DCMAKE_TOOLCHAIN_FILE="$EMSCRIPTEN/cmake/Modules/Platform/Emscripten.cmake" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
cp ../index.html index.html
