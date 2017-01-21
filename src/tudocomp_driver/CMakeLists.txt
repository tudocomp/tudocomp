add_executable(
    tudocomp_driver

    tudocomp_driver.cpp
)

target_link_libraries(
    tudocomp_driver

    tudocomp
    tudocomp_algorithms
    glog
    sdsl
)

cotire(tudocomp_driver)

add_custom_command(TARGET tudocomp_driver POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/tudocomp_driver ${CMAKE_BINARY_DIR})
