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

# docs-main target
find_package(Pandoc)
if(PANDOC_FOUND)
add_custom_target(docs-main
# convert markdown doc to pdf via latex
${PANDOC_BIN} -f markdown -t latex -o ${CMAKE_CURRENT_BINARY_DIR}/docs/Index.pdf --toc --number-sections --listings ${CMAKE_CURRENT_SOURCE_DIR}/docs/Index.md
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
COMMENT "Generating main documentation" VERBATIM
)
endif(PANDOC_FOUND)

# docs-doxygen target
find_package(Doxygen)
if(DOXYGEN_FOUND)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
add_custom_target(docs-doxygen
#invoke doxygen
${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
# make doxygen PDF
COMMAND cd ${CMAKE_CURRENT_BINARY_DIR}/docs/doxygen/latex && make
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
COMMENT "Generating doxygen documentation"
)
endif(DOXYGEN_FOUND)

# docs target (invokes docs-main and docs-doxygen)
if(DOXYGEN_FOUND AND PANDOC_FOUND)
add_custom_target(docs DEPENDS docs-main docs-doxygen)
endif(DOXYGEN_FOUND AND PANDOC_FOUND)

include(compare_workspace)
