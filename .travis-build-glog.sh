#!/bin/bash
set -e

git clone https://github.com/google/glog.git
mkdir -p $GLOG_ROOT_INSTALL
pushd $GLOG_ROOT
  mkdir build
  pushd build
    cmake -DCMAKE_INSTALL_PREFIX:PATH=$GLOG_ROOT_INSTALL ..
    make
    make install
  popd build
popd
