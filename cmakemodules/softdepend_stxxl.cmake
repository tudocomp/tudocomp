# STXXL
ExternalProject_Add(
    get_stxxl PREFIX external/stxxl
    GIT_REPOSITORY https://github.com/stxxl/stxxl
    GIT_TAG b9e44f0ecba7d7111fbb33f3330c3e53f2b75236 # master as of Nov 15, 2018
	CMAKE_ARGS -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    UPDATE_COMMAND ""
    EXCLUDE_FROM_ALL 1
)

find_package(STXXL)
if(STXXL_FOUND)
  include_directories(SYSTEM ${STXXL_INCLUDE_DIRS})
else()
    add_dependencies(get_alldeps get_stxxl)
    MESSAGE(STATUS
    "    STXXL is required for some functionality. "
    "Use -DSTXXL_ROOT_DIR=<path> to point to an existing STXXL installation or "
    "use 'make get_stxxl' to download and install STXXL locally")
endif()

