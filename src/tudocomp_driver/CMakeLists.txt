add_library(
    tudocomp_algorithms

    tudocomp_algorithms.cpp
)

target_link_libraries(
    tudocomp_algorithms

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

    tudocomp
    tudocomp_algorithms
    gflags
    glog
    sdsl
)

add_custom_command(TARGET tudocomp_driver POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/tudocomp_driver ${CMAKE_BINARY_DIR})

