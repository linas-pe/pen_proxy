#!/bin/bash

source .github/bin/common.sh

build_type=$(basename "$0")
build_args="-B build -S . -DPEN_LIBRARY_PATH=${GITHUB_WORKSPACE}/libpen"
if [ "$build_type" == "build_dev.sh" ]; then
    build_args="${build_args} -DCMAKE_BUILD_TYPE=Debug -DPEN_LOCAL_GTEST=ON"
else
    build_args="${build_args} -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${workdir}"
fi

cmake ${build_args} || exit 1
cmake --build build || exit 1

if [ "$build_type" == "build.sh" ]; then
    cmake --build build --target install || exit 1
else
    if [ "$os" == "windows" ]; then
        cmake --build build --target run_tests || exit 1
    else
        cmake --build build --target test || exit 1
    fi
fi

