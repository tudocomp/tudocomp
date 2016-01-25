#!/bin/bash
set -e
set -x

GLOG_ROOT="$TRAVIS_BUILD_DIR/../glog"
GLOG_ROOT_INSTALL="$GLOG_ROOT-install"
rm -rf $GLOG_ROOT
rm -rf $GLOG_ROOT_INSTALL
mkdir -p $GLOG_ROOT
mkdir -p $GLOG_ROOT_INSTALL
pushd $GLOG_ROOT
  git clone https://github.com/google/glog.git .
  mkdir build
  pushd build
    cmake -DCMAKE_INSTALL_PREFIX:PATH=$GLOG_ROOT_INSTALL ..
    make
    make install
  popd
popd
mv $GLOG_ROOT_INSTALL/include/* $GLOG_ROOT_INSTALL/

rm -rf build
mkdir build
cd build
cmake -DGLOG_ROOT_DIR="$GLOG_ROOT_INSTALL" ..
make -j4
make check
