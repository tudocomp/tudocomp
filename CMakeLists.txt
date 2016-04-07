cmake_minimum_required(VERSION 2.8)
project (tudocomp)

include(ExternalProject)

# Using C++11, more warnings and debug info
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++11")
#add_compile_options(-g -Wall -std=c++11) #-fdiagnostics-color=auto -fdump-tree-all)

# Global dependencies
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmakemodules")

include(find_or_download_package)

set(Boost_USE_STATIC_LIBS ON)
find_or_download_package(Boost Boost boost COMPONENTS system program_options filesystem)
find_or_download_package(Glog GLOG glog)
find_or_download_package(SDSL SDSL sdsl)

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

# add a target to generate documentation with Doxygen and Pandoc
find_package(Doxygen)
find_package(Pandoc)

if(DOXYGEN_FOUND AND PANDOC_FOUND)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
add_custom_target(docs #ALL
${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
COMMAND ${PANDOC_BIN} -f markdown -t latex -o ${CMAKE_CURRENT_BINARY_DIR}/docs/Index.pdf ${CMAKE_CURRENT_SOURCE_DIR}/docs/Index.md
COMMENT "Generating documentation" VERBATIM
)
add_dependencies(docs sdsl_external)
endif(DOXYGEN_FOUND AND PANDOC_FOUND)

include(compare_workspace)
