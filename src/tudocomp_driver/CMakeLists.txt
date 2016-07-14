add_library(
    tudocomp_algorithms

    tudocomp_algorithms.cpp
)

target_link_libraries(
    tudocomp_algorithms

    tudostat
    malloc_count
    tudocomp
    glog
    sdsl
)

add_executable(
    tudocomp_driver

    tudocomp_driver.cpp
)

target_link_libraries(
    tudocomp_driver

    tudostat
    tudocomp
    tudocomp_algorithms
    gflags
    glog
    sdsl
)
