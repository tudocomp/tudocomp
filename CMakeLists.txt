cmake_minimum_required(VERSION 3.0.2)
project (tudocomp)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif(NOT CMAKE_BUILD_TYPE)

if(NOT VERSION)
    string(TIMESTAMP DATE "%y%m%d")
    set(VERSION "0.${DATE}-$ENV{USER}${VERSION_SUFFIX}")
endif(NOT VERSION)

message("VERSION = ${VERSION}")

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
# TODO: Add --pedantic and fix issues
# TODO: Actually add -Wall -Wextra -Werror -pedantic
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=${CXX_STANDARD}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb -DDEBUG")

find_package(Boost)

# Paranoid debugging
IF(CMAKE_BUILD_TYPE STREQUAL "Debug" AND PARANOID )
    message("[CAUTION] Paranoid debugging is active!")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DPARANOID")
endif(CMAKE_BUILD_TYPE STREQUAL "Debug" AND PARANOID)

# Disable statistics tracking
if(STATS_DISABLED)
    message("[CAUTION] Statistic tracking is disabled!")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DSTATS_DISABLED")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DSTATS_DISABLED")
endif(STATS_DISABLED)

# Global dependencies
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmakemodules")

# Cotire
include(cotire)
set (COTIRE_MINIMUM_NUMBER_OF_TARGET_SOURCES 1000)

# Finders
include(FindPythonModule)
include(find_or_download_package)

# Usage: find_or_download_package(PACKAGE_NAME, FOUND_FLAG_PREFIX, TARGET_PREFIX)
find_or_download_package(Glog GLOG glog)
find_or_download_package(SDSL SDSL sdsl)

include(FindJudy)

# Generate source files
include(generate_tudocomp_sources)

#### All major project targets ####

#include(sdsl)

# Add include directory to include searches
include_directories(include)

# TuDoComp library and implementors
add_subdirectory(src)

# Unit tests for everything in src
add_subdirectory(test)

# etc
add_subdirectory(etc)

# Docs
add_subdirectory(docs)

# Web
add_subdirectory(www)

add_library(
    tudocomp_algorithms
    ${CMAKE_BINARY_DIR}/tudocomp_algorithms.cpp
)

target_link_libraries(
    tudocomp_algorithms

    tudocomp_stat
    tudocomp
    glog
    sdsl
)

cotire(tudocomp_algorithms)


MESSAGE( STATUS "Built Type: " ${CMAKE_BUILD_TYPE} )
MESSAGE( STATUS "CXX Flags: " ${CMAKE_CXX_FLAGS} )
MESSAGE( STATUS "[Optional] Judy Array: " ${JUDY_H_AVAILABLE} )
MESSAGE( STATUS "[Optional] Boost: " ${Boost_FOUND} )

