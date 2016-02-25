add_executable(
    tudocomp_driver

    tudocomp_algorithms.cpp
    tudocomp_driver.cpp
)

target_link_libraries(
    tudocomp_driver
    tudocomp
    glog
    sdsl
    boost_program_options
)
