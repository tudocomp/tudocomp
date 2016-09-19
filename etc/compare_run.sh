#!/bin/bash
# abort on non-zero exit of command
set -e
pushd ..
make tudocomp_driver -j4
make compare_tool
make compare_workspace
popd > /dev/null
export PATH=../src/tudocomp_driver/:$PATH
export DATASETS=../../etc/datasets
../src/compare_tool/debug/compare_tool $@
