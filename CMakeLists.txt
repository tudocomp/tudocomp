cmake_minimum_required(VERSION 3.0.2)
project (tudocomp)

string(TIMESTAMP CURRENT_REVISION "%Y.%m.%d.%H%M")
set(VERSION "0.${CURRENT_REVISION}")

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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=${CXX_STANDARD}")

# Global dependencies
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmakemodules")

# Cotire
include(cotire)
set (COTIRE_MINIMUM_NUMBER_OF_TARGET_SOURCES 1)

# Finders
include(FindPythonModule)
include(find_or_download_package)

# Usage: find_or_download_package(PACKAGE_NAME, FOUND_FLAG_PREFIX, TARGET_PREFIX)
find_or_download_package(Glog GLOG glog)
find_or_download_package(SDSL SDSL sdsl)
find_or_download_package(GFlags GFLAGS gflags)

include(FindJudy)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/include/tudocomp/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/tudocomp/config.h)


#include(sdsl)
include(rust)

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

include(compare_workspace)




add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tudocomp_algorithms.cpp ${CMAKE_CURRENT_SOURCE_DIR}/include/tudocomp/tudocomp.hpp
	COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/genregistry.sh ${CMAKE_CURRENT_BINARY_DIR}/tudocomp_algorithms.cpp ${CMAKE_CURRENT_SOURCE_DIR}/include/tudocomp/tudocomp.hpp ${CMAKE_CURRENT_SOURCE_DIR}/include/tudocomp/config.h
	COMMENT "Generating tudocomp_algorithms.cpp and tudocomp.hpp..."
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/genregistry.sh ${CMAKE_CURRENT_SOURCE_DIR}/include/tudocomp/config.h
)

add_library(
    tudocomp_algorithms
	${CMAKE_CURRENT_BINARY_DIR}/tudocomp_algorithms.cpp
)

target_link_libraries(
    tudocomp_algorithms

    malloc_count
    tudocomp
    glog
    sdsl
)

cotire(tudocomp_algorithms)




