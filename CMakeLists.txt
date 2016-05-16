cmake_minimum_required(VERSION 2.8)
project (tudocomp)

include(ExternalProject)

# Using C++11, more warnings and debug info
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=gnu++11")

# Global dependencies
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmakemodules")

include(find_or_download_package)

# Usage: find_or_download_package(PACKAGE_NAME, FOUND_FLAG_PREFIX, TARGET_PREFIX)
find_or_download_package(Glog GLOG glog)
find_or_download_package(SDSL SDSL sdsl)
find_or_download_package(GFlags GFLAGS gflags)

#include(sdsl)
include(rust)

# Add include directory to include searches
include_directories(include)

# TuDoComp library and implementors
add_subdirectory(src)

# Unit tests for everything in src
add_subdirectory(test)

# Huge optional corpus of text files for comparisons
add_subdirectory(datasets)

# Docs
add_subdirectory(docs)

include(compare_workspace)
