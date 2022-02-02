# - Try to find TDC
#
# The following variables are optionally searched for defaults
#  TDC_ROOT_DIR: Base directory where all TDC components are found
#
# The following are set after configuration is done:
#  TDC_FOUND
#  TDC_INCLUDE_DIRS
#  TDC_LIBRARIES

include(FindPackageHandleStandardArgs)

set(TDC_ROOT_DIR "" CACHE PATH "Folder contains tdc")
set(TDC_LOCAL_DIR "${CMAKE_BINARY_DIR}/external/tdc")

find_path(TDC_INCLUDE_DIR tdc/namespace.hpp PATHS ${TDC_ROOT_DIR} ${TDC_LOCAL_DIR} PATH_SUFFIXES include)
find_library(TDC_PRED_LIBRARY tdc-pred PATHS ${TDC_ROOT_DIR} ${TDC_LOCAL_DIR} PATH_SUFFIXES build/src/pred)

find_package_handle_standard_args(TDC DEFAULT_MSG TDC_INCLUDE_DIR TDC_PRED_LIBRARY)

if(TDC_FOUND)
    set(TDC_INCLUDE_DIRS ${TDC_INCLUDE_DIR})
    set(TDC_LIBRARIES ${TDC_PRED_LIBRARY})
endif()
