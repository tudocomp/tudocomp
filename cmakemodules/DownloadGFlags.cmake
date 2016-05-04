ExternalProject_Add(
    gflags_external
    GIT_REPOSITORY https://github.com/gflags/gflags.git
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    UPDATE_COMMAND ""
)
ExternalProject_Get_Property(gflags_external source_dir install_dir)

file(MAKE_DIRECTORY "${install_dir}/include")

set(${package_found_prefix}_CMAKE_DEP gflags_external)
set(${package_found_prefix}_LIBRARIES "${install_dir}/lib/libgflags_nothreads.a")
set(${package_found_prefix}_INCLUDE_DIRS "${install_dir}/include")
