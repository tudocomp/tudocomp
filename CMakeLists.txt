cmake_minimum_required(VERSION 3.2)
project (tudocomp VERSION 0.1)

include(ExternalProject)

# Using C++11, more wanrnings and debug info
add_compile_options(-g -Wall -std=c++11) #-fdump-tree-all)

# Global dependencies
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmakemodules")
find_package(Boost REQUIRED)
find_package(Glog REQUIRED)
include(sdsl)

# TuDoComp library and implementors
add_subdirectory(src)

# Unit tests for everything in src
add_subdirectory(test)

# Huge optional corpus of text files for comparisions
add_subdirectory(datasets)

# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
add_custom_target(docs #ALL
${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
COMMENT "Generating API documentation with Doxygen" VERBATIM
)
add_dependencies(docs sdsl_external)
endif(DOXYGEN_FOUND)
