# ccache
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

# set build type
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "no CMAKE_BUILD_TYPE given -- setting to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif(NOT CMAKE_BUILD_TYPE)

# set universal compiler flags
set(CXX_STANDARD c++20)

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    # using Clang or AppleClang
    if(${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS "10")
        message(FATAL_ERROR "clang version 10 or greater required!")
    endif()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=${CXX_STANDARD}")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # using GCC
    if(${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS "10")
        message(FATAL_ERROR "g++ version 10 or greater required!")
    endif()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++20")
else()
    message(FATAL_ERROR "compiler ${CMAKE_CXX_COMPILER_ID} is not supported!")
endif()

set(GCC_WARNINGS "-Wall -Werror=return-type")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread ${GCC_WARNINGS} -fdiagnostics-color=auto")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb -DDEBUG ${SANITIZATION}")

# Cotire
include(cotire)
set (COTIRE_MINIMUM_NUMBER_OF_TARGET_SOURCES 1000)

# ExternalProject
include(ExternalProject)

# Dependency fetchers
add_custom_target(get_deps)

add_custom_target(get_alldeps)
add_dependencies(get_alldeps get_deps)

# Test if dependencies are missing and if so, quit
macro(tdc_check_hard_deps)
    if(TDC_DEPS_MISSING)
        MESSAGE(WARNING
        "There are missing hard dependencies! See above for details. "
        "You may use 'make get_deps' to download and install these "
        "dependencies locally.")
    endif()
endmacro()

