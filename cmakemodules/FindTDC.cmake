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
set(TDC_MIGRATION_DIR "${CMAKE_BINARY_DIR}/tdc_external-prefix")

find_path(TDC_INCLUDE_DIR namespace.h
    PATHS ${TDC_ROOT_DIR} ${TDC_LOCAL_DIR} ${TDC_MIGRATION_DIR}
    PATH_SUFFIXES include)

#find_path(TDC_BUILD_INCLUDE_DIR tdc/bits/config.h
#    PATHS ${TDC_ROOT_DIR} ${TDC_LOCAL_DIR} ${TDC_MIGRATION_DIR}
#    PATH_SUFFIXES include build/include)

find_library(TDC_LIBRARY tdc
    PATHS ${TDC_ROOT_DIR} ${TDC_LOCAL_DIR} ${TDC_MIGRATION_DIR}
    PATH_SUFFIXES lib build/lib)

find_package_handle_standard_args(TDC DEFAULT_MSG
    TDC_INCLUDE_DIR TDC_LIBRARY)

if(TDC_FOUND)
    set(TDC_INCLUDE_DIRS ${TDC_INCLUDE_DIR} ${TDC_BUILD_INCLUDE_DIR})
    set(TDC_LIBRARIES ${TDC_LIBRARY})
endif()
