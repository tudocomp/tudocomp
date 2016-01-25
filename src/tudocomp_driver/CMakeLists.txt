#[[
# This uses the C++ implementation of docopt for its command line interface
ExternalProject_Add(
    docopt
    GIT_REPOSITORY https://github.com/docopt/docopt.cpp.git
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    UPDATE_COMMAND ""
)
add_library(libdocopt IMPORTED STATIC GLOBAL)
add_dependencies(libdocopt docopt)

# Set docopt properties
ExternalProject_Get_Property(docopt source_dir install_dir)
set_target_properties(libdocopt PROPERTIES
    "IMPORTED_LOCATION" "${install_dir}/lib/libdocopt_s.a"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
    #"INTERFACE_INCLUDE_DIRECTORIES" "${source_dir}/include"
)
include_directories("${install_dir}/include")
]]

add_library(tudocomp_algorithms tudocomp_algorithms.cpp)

target_link_libraries(
    tudocomp_algorithms

    ${Boost_LIBRARIES}
    custom_glog
    tudocomp

    # Depend on all algorithms
    dummy
    esacomp
    lz_compressor
)
target_include_directories(tudocomp_algorithms PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(
    tudocomp_driver

    main.cpp
)

target_link_libraries(
    tudocomp_driver

    tudocomp_algorithms

    #libdocopt
    ${Boost_LIBRARIES}
    custom_glog
    tudocomp
)
