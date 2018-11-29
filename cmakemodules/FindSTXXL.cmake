# - Try to find STXXL
#
# The following variables are optionally searched for defaults
#  STXXL_ROOT_DIR: Base directory where all STXXL components are found
#
# The following are set after configuration is done:
#  STXXL_FOUND
#  STXXL_INCLUDE_DIRS
#  STXXL_LIBRARIES

include(FindPackageHandleStandardArgs)

set(STXXL_ROOT_DIR "" CACHE PATH "Folder contains stxxl")

find_path(STXXL_INCLUDE_DIR stxxl.h PATHS ${STXXL_ROOT_DIR}/include)
find_path(STXXL_BUILD_INCLUDE_DIR stxxl/bits/config.h PATHS ${STXXL_ROOT_DIR}/build/include)

find_library(STXXL_LIBRARY stxxl PATHS ${STXXL_ROOT_DIR}/build/lib)

find_package_handle_standard_args(Stxxl DEFAULT_MSG
    STXXL_INCLUDE_DIR STXXL_LIBRARY)

if(STXXL_FOUND)
    set(STXXL_INCLUDE_DIRS ${STXXL_INCLUDE_DIR} ${STXXL_BUILD_INCLUDE_DIR})
    set(STXXL_LIBRARIES ${STXXL_LIBRARY})
endif()
