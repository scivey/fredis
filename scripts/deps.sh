#!/bin/bash

ROOT=$(git rev-parse --show-toplevel)

EXT=${ROOT}/external

function cmake-build() {
    if [[ ! -d "build" ]]; then
        mkdir build
        pushd build
        cmake ../ && make -j8
        popd
    fi
}

pushd ${EXT}/googletest
cmake-build
popd

