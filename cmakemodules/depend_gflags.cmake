# Find or fetch gflags
ExternalProject_Add(
    get_gflags PREFIX external/gflags
    GIT_REPOSITORY https://github.com/gflags/gflags
    GIT_TAG e171aa2d15ed9eb17054558e0b3a6a413bb01067 # v2.2.2
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    UPDATE_COMMAND ""
    EXCLUDE_FROM_ALL 1
)
find_package(Gflags)

if(GFLAGS_FOUND)
    include_directories(${GFLAGS_INCLUDE_DIRS})
else()
    set(TDC_DEPS_MISSING 1)
    add_dependencies(get_deps get_gflags)
    message(WARNING
    "gflags is REQUIRED! "
    "Install the libgflags-dev package on your system, "
    "use -DGFLAGS_ROOT_DIR=<path> to point to an existing gflags installation "
    "or use 'make get_gflags' to download and install gflags locally")
endif()

