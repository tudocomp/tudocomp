ExternalProject_Add(
    sdsl_external
    GIT_REPOSITORY https://github.com/simongog/sdsl-lite.git
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    UPDATE_COMMAND ""
)
ExternalProject_Get_Property(sdsl_external source_dir install_dir)

file(MAKE_DIRECTORY "${install_dir}/include")

set(${package_found_prefix}_CMAKE_DEP sdsl_external)
set(${package_found_prefix}_LIBRARIES
    "${install_dir}/lib/libdivsufsort.a"
    "${install_dir}/lib/libdivsufsort64.a"
    "${install_dir}/lib/libsdsl.a"
)
set(${package_found_prefix}_INCLUDE_DIRS "${install_dir}/include")
