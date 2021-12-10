include(depend_gflags)

# Find or fetch glog
ExternalProject_Add(
    get_glog PREFIX external/glog
    GIT_REPOSITORY https://github.com/google/glog.git
    GIT_TAG a6a166db069520dbbd653c97c2e5b12e08a8bb26 # v0.3.5
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    UPDATE_COMMAND ""
    EXCLUDE_FROM_ALL 1
)
find_package(Glog)

if(GLOG_FOUND)
    include_directories(${GLOG_INCLUDE_DIRS})
else()
    set(TDC_DEPS_MISSING 1)
    add_dependencies(get_deps get_glog)
    message(WARNING
    "Google logging (glog) is REQUIRED! "
    "Install the libgoogle-glog-dev package on your system, "
    "use -DGLOG_ROOT_DIR=<path> to point to an existing glog installation "
    "or use 'make get_glog' to download and install glog locally")
endif()

