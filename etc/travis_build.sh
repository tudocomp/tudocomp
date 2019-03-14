#!/bin/bash
set -e
set -x

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make get_alldeps
cmake ..
make build_check
make
make check
