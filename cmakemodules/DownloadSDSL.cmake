ExternalProject_Add(
    sdsl_external
    GIT_REPOSITORY https://github.com/simongog/sdsl-lite.git
    GIT_TAG 7bbb71e8e13279ab45111bd135f4210545cd1c85
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    UPDATE_COMMAND ""
    PATCH_COMMAND git apply ${CMAKE_SOURCE_DIR}/etc/cygwin-sdsl.gitpatch
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
