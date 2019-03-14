cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)
project (tudocomp)

# init build system
execute_process(COMMAND git submodule update --init -- build_system
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/submodules)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/submodules/build_system/cmakemodules")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmakemodules")

include(tdc_init)

# hard dependencies
set(Python_ADDITIONAL_VERSIONS 3)
find_package(PythonInterp REQUIRED)

# downloadable dependencies
include(depend_glog)

# soft dependencies
include(softdepend_gtest)
include(softdepend_sdsl)
include(softdepend_stxxl)

# quit if dependencies aren't met
tdc_check_hard_deps()
if(TDC_DEPS_MISSING)
    return()
endif()

list(APPEND TDC_DEPENDS ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES})

# submodules
include(git_submodule_subdirectories)
git_submodule_subdirectory(submodules/build_system)
git_submodule_subdirectory(submodules/bit_span)
git_submodule_subdirectory(submodules/tudostats)
git_submodule_subdirectory(submodules/compact_sparse_hash)
git_submodule_subdirectory(submodules/datasets)

# judy
include(FindJudy)

# find OpenMP
find_package(OpenMP)
include(FindOpenMP)
if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS} -DENABLE_OPENMP")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif(OPENMP_FOUND)

# set integer width for length type
if(NOT DEFINED LEN_BITS)
    set(LEN_BITS 32)
endif(NOT DEFINED LEN_BITS)

message(STATUS "Setting length type to ${LEN_BITS} bits (-DLEN_BITS=${LEN_BITS})")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLEN_BITS=${LEN_BITS}")

# paranoid debugging
IF(CMAKE_BUILD_TYPE STREQUAL "Debug" AND PARANOID )
    message("[CAUTION] Paranoid debugging is active!")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DPARANOID")
endif(CMAKE_BUILD_TYPE STREQUAL "Debug" AND PARANOID)

# disable stat tracking / malloc count
if(STATS_DISABLED)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DSTATS_DISABLED")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DSTATS_DISABLED")
endif(STATS_DISABLED)

if(MALLOC_DISABLED)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DMALLOC_DISABLED")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DMALLOC_DISABLED")
endif(MALLOC_DISABLED)

# generate source files
include(generate_tudocomp_sources)

#### All major project targets ####

# Add include directory to include searches
include_directories(include)

# TuDoComp library and implementors
if(DEFINED TDC_REGISTRY)
    get_filename_component(TDC_REGISTRY ${TDC_REGISTRY} ABSOLUTE
        BASE_DIR ${CMAKE_BINARY_DIR})

    message(STATUS "[CAUTION] Using custom registry config: ${TDC_REGISTRY}")
else()
    set(TDC_REGISTRY ${CMAKE_SOURCE_DIR}/etc/registries/all_algorithms.py)
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
MESSAGE( STATUS "[Optional] SDSL: " ${SDSL_FOUND} )
MESSAGE( STATUS "[Optional] STXXL: " ${STXXL_FOUND} )

# for showing include in qtcreator
FILE(GLOB_RECURSE LibFiles "include/*.hpp")
add_custom_target(headers SOURCES ${LibFiles})

