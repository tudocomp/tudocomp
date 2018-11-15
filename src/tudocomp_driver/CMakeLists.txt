add_executable(
    tudocomp_driver

    tudocomp_driver.cpp
)

add_dependencies(
    tudocomp_driver

    generate_version
)

target_link_libraries(
    tudocomp_driver

    tudocomp
    tudocomp_algorithms
    glog
    sdsl
    pthread
    gomp
    bit_span
    compact_sparse_hash
)

cotire(tudocomp_driver)

add_custom_command(TARGET tudocomp_driver POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/tudocomp_driver ${CMAKE_BINARY_DIR}/tdc)
