#!/bin/bash
set -e
set -x

mkdir -p $GLOG_ROOT_INSTALL
mkdir -p $GLOG_ROOT
pushd $GLOG_ROOT
  git clone https://github.com/google/glog.git .
  mkdir build
  pushd build
    cmake -DCMAKE_INSTALL_PREFIX:PATH=$GLOG_ROOT_INSTALL ..
    make
    make install
  popd build
popd
