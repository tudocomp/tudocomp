add_library(
    tudocomp_algorithms

    tudocomp_algorithms.cpp
)

target_link_libraries(
    tudocomp_algorithms

    boost
    tudocomp
    glog
    sdsl
    boost_program_options
)

add_executable(
    tudocomp_driver

    tudocomp_driver.cpp
)

target_link_libraries(
    tudocomp_driver

    boost
    tudocomp
    tudocomp_algorithms
    gflags
    glog
    sdsl
    boost_program_options
)
