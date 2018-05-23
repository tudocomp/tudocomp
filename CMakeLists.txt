cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)
project (tudocomp)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif(NOT CMAKE_BUILD_TYPE)

# Check compiler version
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" AND
   ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS "5.4")
    message(FATAL_ERROR "g++ version 5.4 or greater required!")
endif()

set(CXX_STANDARD c++14)

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    # using Clang or AppleClang
    if(${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS "3.4")
        message(FATAL_ERROR "clang version 3.4 or greater required!")
    endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # using GCC
    if(${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS "5.4")
        message(FATAL_ERROR "g++ version 5.4 or greater required!")
    endif()
else()
    message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER_ID} is not supported!")
endif()

include(ExternalProject)

# More warnings and debug info
# TODO: Add these, they break building glog -Wextra -Wpedantic
set(GCC_WARNINGS "-Wall -Werror=return-type")
#set(SANITIZATION "-fsanitize=undefined -fsanitize=address -fsanitize=leak")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14 ${GCC_WARNINGS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb -DDEBUG ${SANITIZATION}")

# Set integer with for length type
if(DEFINED LEN_BITS)
    message(STATUS "Setting length type to ${LEN_BITS} Bits")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLEN_BITS=${LEN_BITS}")
endif(DEFINED LEN_BITS)

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

# Find Python3
set(Python_ADDITIONAL_VERSIONS 3)
find_package(PythonInterp REQUIRED)

# Global dependencies
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmakemodules")

# Cotire
include(cotire)
set (COTIRE_MINIMUM_NUMBER_OF_TARGET_SOURCES 1000)

# Git Submodule sources
include(git_submodule_subdirectories)

# Finders
include(find_or_download_package)

# Usage: find_or_download_package(PACKAGE_NAME, FOUND_FLAG_PREFIX, TARGET_PREFIX)
find_or_download_package(Glog GLOG glog)
find_or_download_package(SDSL SDSL sdsl)
git_submodule_subdirectory(external/bit_span)
git_submodule_subdirectory(external/compact_sparse_hash)
git_submodule_subdirectory(external/datasets)

include(FindJudy)

# Generate source files
include(generate_tudocomp_sources)

#### All major project targets ####

#include(sdsl)


# Add include directory to include searches
include_directories(include)

# TuDoComp library and implementors
if(DEFINED TDC_REGISTRY)
    message("[CAUTION] Using custom registry config: ${TDC_REGISTRY}")
else()
    set(TDC_REGISTRY ${CMAKE_SOURCE_DIR}/etc/registry_config.py)
endif(DEFINED TDC_REGISTRY)

add_subdirectory(src)

# Unit tests for everything in src
add_subdirectory(test)

# Examples
add_subdirectory(examples)

# etc
add_subdirectory(etc)

# Docs
add_subdirectory(docs)

# Web
add_subdirectory(www)

MESSAGE( STATUS "Built Type: " ${CMAKE_BUILD_TYPE} )
MESSAGE( STATUS "[Optional] Judy Array: " ${JUDY_H_AVAILABLE} )
MESSAGE( STATUS "[Optional] Boost: " ${Boost_FOUND} )

# for showing include in qtcreator
FILE(GLOB_RECURSE LibFiles "include/*.hpp")
add_custom_target(headers SOURCES ${LibFiles})
