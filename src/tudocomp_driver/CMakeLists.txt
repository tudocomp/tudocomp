add_library(tudocomp_algorithms tudocomp_algorithms.cpp)

target_link_libraries(
    tudocomp_algorithms

    boost
    glog
    tudocomp

    # Depend on all algorithms
    esacomp
    esacomp_impl
    esacomp_dummy
    esacomp_lz_factor
    lz78
    lz78_impl
    lzw
    lzw_impl
)
target_include_directories(tudocomp_algorithms PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(
    tudocomp_driver

    main.cpp
)

target_link_libraries(
    tudocomp_driver

    tudocomp_algorithms

    boost
    glog
    tudocomp
)
