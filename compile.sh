#!/bin/sh
CMAKE_OPTS=$@

mkdir -p build && cd build
cmake $CMAKE_OPTS ..
make get_alldeps
cmake ..
make

