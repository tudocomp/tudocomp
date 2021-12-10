# - Try to find Glog
#
# The following variables are optionally searched for defaults
#  GLOG_ROOT_DIR:            Base directory where all GLOG components are found
#
# The following are set after configuration is done:
#  GLOG_FOUND
#  GLOG_INCLUDE_DIRS
#  GLOG_LIBRARIES
#  GLOG_LIBRARYRARY_DIRS

include(FindPackageHandleStandardArgs)

set(GLOG_ROOT_DIR "" CACHE PATH "Folder contains Google glog")
set(GLOG_LOCAL_DIR "${CMAKE_BINARY_DIR}/external/glog")

find_path(GLOG_INCLUDE_DIR glog/logging.h
    PATHS ${GLOG_ROOT_DIR} ${GLOG_LOCAL_DIR}
    PATH_SUFFIXES include)

find_library(GLOG_LIBRARY glog
    PATHS ${GLOG_ROOT_DIR} ${GLOG_LOCAL_DIR}
    PATH_SUFFIXES lib)

find_package_handle_standard_args(Glog DEFAULT_MSG
    GLOG_INCLUDE_DIR GLOG_LIBRARY)

if(GLOG_FOUND)
    set(GLOG_INCLUDE_DIRS ${GLOG_INCLUDE_DIR})
    set(GLOG_LIBRARIES ${GLOG_LIBRARY})
endif()
