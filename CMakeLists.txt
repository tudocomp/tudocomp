cmake_minimum_required(VERSION 3.2)
project (tudocomp VERSION 0.1)

include(ExternalProject)

# Using C++11, more warnings and debug info
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++11")
#add_compile_options(-g -Wall -std=c++11) #-fdiagnostics-color=auto -fdump-tree-all)

# Global dependencies
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmakemodules")
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED system program_options filesystem)
find_package(Glog REQUIRED)

add_library(custom_glog IMPORTED STATIC GLOBAL)
set_target_properties(custom_glog PROPERTIES
    "IMPORTED_LOCATION" "${GLOG_LIBRARIES}"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
    "INTERFACE_INCLUDE_DIRECTORIES" "${GLOG_INCLUDE_DIRS}"
)

include(sdsl)
include(rust)

# TuDoComp library and implementors
add_subdirectory(src)

# Unit tests for everything in src
add_subdirectory(test)

# Huge optional corpus of text files for comparisons
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

include(compare_workspace)
