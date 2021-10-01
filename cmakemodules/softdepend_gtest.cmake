# GTEST
# Repository versions of gtest are not compiled using -fPIC, which causes
# problems. To be on the safe side, we always build a local install of gtest.
ExternalProject_Add(
    get_gtest PREFIX external/gtest
    GIT_REPOSITORY https://github.com/google/googletest
    GIT_TAG e2239ee6043f73722e7aa812a459f54a28552929 # 1.11.0
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    UPDATE_COMMAND ""
    EXCLUDE_FROM_ALL 1
)

set(GTEST_ROOT "${CMAKE_BINARY_DIR}/external/gtest" CACHE PATH "Path to googletest")
find_package(GTest)

if(GTEST_FOUND)
    include_directories(${GTEST_INCLUDE_DIRS})
else()
    add_dependencies(get_alldeps get_gtest)
    message(STATUS
    "    Googletest (gtest) is required for running the tests. "
    "Use -DGTEST_ROOT=<path> to point to an existing gtest installation "
    "or use 'make get_gtest' to download and install gtest locally.")
    message(STATUS
    "    We do NOT recommend installing the libgtest-dev package -- "
    "it is not built using -fPIC and you will run into linker errors.")
endif()

