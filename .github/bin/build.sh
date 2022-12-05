#!/bin/bash

source .github/bin/common.sh

mkdir -p m4 build
(
    cd m4 || exit 1
    libtoolize || exit 1
)
aclocal || exit 1
autoheader || exit 1
automake --add-missing || exit 1
autoconf || exit 1

build_type=$(basename "$0")
(
    cd build || exit 1
    if [ "$build_type" == "build_dev.sh" ]; then
        ../configure  --enable-debug || exit 1
    else
        ../configure --prefix=/usr || exit 1
    fi
)

make || exit 1

if [ "$build_type" == "build.sh" ]; then
    make install prefix="${workdir}"
fi

