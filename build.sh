#!/bin/bash

ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD="${ROOT}/build"

mkdir -p ${BUILD}

pushd ${BUILD}
rustc ${ROOT}/sol.rs
popd
