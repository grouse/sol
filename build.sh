#!/bin/bash


ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD="${ROOT}/build"

mkdir -p ${BUILD}

pushd ${BUILD}
#rustc -g -C opt-level=2 ${ROOT}/sol.rs
clang++ -Wall -Wextra -g -O3 -o sol -lpthread ${ROOT}/sol.cpp
./sol
xdg-open ./test.bmp
popd
