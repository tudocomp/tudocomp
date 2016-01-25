#!/bin/bash
set -e
set -x

mkdir build
cd build
cmake ..
make -j4
make check
