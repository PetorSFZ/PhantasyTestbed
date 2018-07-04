#!/bin/bash

# Set PhantasyTestbed main directory as working directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/..

# Delete old build directory and create new one
rm -rf build_ninja_release
mkdir build_ninja_release
cd build_ninja_release

# Generate build files
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release

# Run create_symlinks.sh shell script
chmod +x create_symlinks.sh
./create_symlinks.sh
