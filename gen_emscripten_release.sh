#!/bin/bash
rm -rf build_emscripten_release
mkdir build_emscripten_release
cd build_emscripten_release
cmake .. -DCMAKE_TOOLCHAIN_FILE="$EMSCRIPTEN/cmake/Modules/Platform/Emscripten.cmake" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
cp ../index.html index.html
