# tdc
ExternalProject_Add(
    get_tdc PREFIX external/tdc
    GIT_REPOSITORY https://github.com/pdinklag/tdc
    GIT_TAG eb71cecaa7ea724202e75287465cc989c390ff26 # master as of Dec 13, 2021
#	CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    UPDATE_COMMAND ""
    EXCLUDE_FROM_ALL 1
)

find_package(TDC)
if(TDC_FOUND)
    include_directories(${TDC_INCLUDE_DIRS})
else()
    include_directories(${CMAKE_SOURCE_DIR}/build/external/tdc/src/get_tdc/include)
    add_dependencies(get_alldeps get_tdc)
    MESSAGE(STATUS
    "    tdc is required for some functionality. "
    "use 'make get_tdc' to download and install TDC locally")
endif()

