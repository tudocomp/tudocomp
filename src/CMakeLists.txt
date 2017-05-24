add_subdirectory(tudocomp_stat)
add_subdirectory(tudocomp)
add_subdirectory(tudocomp_driver)
add_subdirectory(generated)



add_executable(
    plcp

    plcp_main.cpp
)

add_dependencies(
    plcp

    generate_version
)

target_link_libraries(
    plcp

    tudocomp
    tudocomp_algorithms
    glog
    sdsl
	pthread
	gomp
)

cotire(plcp)

add_custom_command(TARGET plcp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/plcp ${CMAKE_BINARY_DIR}/plcp)
