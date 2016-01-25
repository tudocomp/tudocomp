ExternalProject_Add(
    sdsl_external
    GIT_REPOSITORY https://github.com/simongog/sdsl-lite.git
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    UPDATE_COMMAND ""
)
ExternalProject_Get_Property(sdsl_external source_dir install_dir)

file(MAKE_DIRECTORY "${install_dir}/include")

add_library(divsufsort IMPORTED STATIC GLOBAL)
add_dependencies(divsufsort sdsl_external)
set_target_properties(divsufsort PROPERTIES
    "IMPORTED_LOCATION" "${install_dir}/lib/libdivsufsort.a"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
    "INTERFACE_INCLUDE_DIRECTORIES" "${install_dir}/include"
)

add_library(divsufsort64 IMPORTED STATIC GLOBAL)
add_dependencies(divsufsort64 sdsl_external)
set_target_properties(divsufsort64 PROPERTIES
    "IMPORTED_LOCATION" "${install_dir}/lib/libdivsufsort64.a"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
    "INTERFACE_INCLUDE_DIRECTORIES" "${install_dir}/include"
)

add_library(sdsl_lib IMPORTED STATIC GLOBAL)
add_dependencies(sdsl_lib sdsl_external divsufsort divsufsort64)
set_target_properties(sdsl_lib PROPERTIES
    "IMPORTED_LOCATION" "${install_dir}/lib/libsdsl.a"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
    "INTERFACE_INCLUDE_DIRECTORIES" "${install_dir}/include"
)
add_library(sdsl INTERFACE)
target_link_libraries(sdsl INTERFACE sdsl_lib divsufsort divsufsort64)
