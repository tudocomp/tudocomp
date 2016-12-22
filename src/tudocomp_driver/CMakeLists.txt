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

cotire(tudocomp_algorithms)

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

cotire(tudocomp_driver)
add_custom_command(OUTPUT tudocomp_algorithms.cpp
	COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/genregistry.sh > ${CMAKE_CURRENT_BINARY_DIR}/tudocomp_algorithms.cpp
	COMMENT "tudocomp_algorithms.cpp..." )


add_custom_command(TARGET tudocomp_driver POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/tudocomp_driver ${CMAKE_BINARY_DIR})

