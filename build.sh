#!/bin/bash


ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD="${ROOT}/build"

mkdir -p ${BUILD}

pushd ${BUILD}
# rustc -g -C opt-level=0 ${ROOT}/sol.rs
clang++ -Wall -Wextra -g -O0 -o sol ${ROOT}/sol.cpp
./sol
xdg-open ./test.bmp
popd
