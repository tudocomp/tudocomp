add_subdirectory(test)

add_library(
    tudocomp

    lz78_impl.cpp
    lzw_impl.cpp
)

target_link_libraries(
    tudocomp
    glog
    sdsl
)

add_executable(
    tudocomp_driver

    lz78_impl.cpp
    lzw_impl.cpp
    tudocomp_algorithms.cpp
    tudocomp.cpp
)

target_link_libraries(
    tudocomp_driver
    glog
    sdsl
    boost_program_options
)
