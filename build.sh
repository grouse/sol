#!/bin/bash


ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD="${ROOT}/build"

mkdir -p ${BUILD}

pushd ${BUILD}
rustc -g -C opt-level=0 ${ROOT}/sol.rs
./sol
xdg-open ./test.bmp
popd
