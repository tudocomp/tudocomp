#!/bin/bash
set -e
set -x

mkdir build
cd build
cmake ..
make
make build_check
make check
