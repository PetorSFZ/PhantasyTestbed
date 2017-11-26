#!/bin/bash

# Set PhantasyTestbed main directory as working directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/..

# Delete old build directory and create new one
rm -rf build_xcode
mkdir build_xcode
cd build_xcode

# Generate build files
cmake .. -GXcode

# Create resources symlinks
mkdir Debug
cd Debug
ln -s ../../resources resources

cd ..
mkdir RelWithDebInfo
cd RelWithDebInfo
ln -s ../../resources resources

cd ..
mkdir Release
cd Release
ln -s ../../resources resources
