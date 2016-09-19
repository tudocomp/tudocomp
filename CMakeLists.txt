cmake_minimum_required(VERSION 3.0.2)
project (tudocomp)

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS "4.9")
    message(FATAL_ERROR "g++ version 4.9 or greater required!")
endif()

include(ExternalProject)

# Select C++ standard
if(${CYGWIN})
    set(CMAKE_LEGACY_CYGWIN_WIN32 0)
    set(CXX_STANDARD gnu++11)
else()
    set(CXX_STANDARD c++11)
endif()

# Using C++11, more warnings and debug info
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=${CXX_STANDARD}")

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
add_subdirectory(etc/datasets)

# Docs
add_subdirectory(docs)

include(compare_workspace)
