# - Try to find GFLAGS
#
# The following variables are optionally searched for defaults
#  GFLAGS_ROOT_DIR:            Base directory where all GFLAGS components are found
#
# The following are set after configuration is done:
#  GFLAGS_FOUND
#  GFLAGS_INCLUDE_DIRS
#  GFLAGS_LIBRARIES
#  GFLAGS_LIBRARYRARY_DIRS

include(FindPackageHandleStandardArgs)

set(GFLAGS_ROOT_DIR "" CACHE PATH "Folder contains Gflags")
set(GFLAGS_LOCAL_DIR "${CMAKE_BINARY_DIR}/external/gflags")

find_path(GFLAGS_INCLUDE_DIR gflags/gflags.h
    PATHS ${GFLAGS_ROOT_DIR} ${GFLAGS_LOCAL_DIR}
    PATH_SUFFIXES include)

find_library(GFLAGS_LIBRARY gflags
    PATHS ${GFLAGS_ROOT_DIR} ${GFLAGS_LOCAL_DIR}
    PATH_SUFFIXES lib)

find_package_handle_standard_args(Gflags DEFAULT_MSG GFLAGS_INCLUDE_DIR GFLAGS_LIBRARY)

if(GFLAGS_FOUND)
    set(GFLAGS_INCLUDE_DIRS ${GFLAGS_INCLUDE_DIR})
    set(GFLAGS_LIBRARIES ${GFLAGS_LIBRARY})
endif()

