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

add_executable(
    plcp-decomp
    plcp_decomp_main.cpp
)

add_dependencies(
    plcp-decomp
    generate_version
)

target_link_libraries(
    plcp-decomp
    tudocomp
    tudocomp_algorithms
    glog
    sdsl
	pthread
	gomp
)

cotire(plcp-decomp)
    
add_custom_command(TARGET plcp-decomp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/plcp-decomp ${CMAKE_BINARY_DIR}/plcp-decomp)

add_executable(
    plcp-decomp2
    plcp_decomp_main2.cpp
)

add_dependencies(
    plcp-decomp2
    generate_version
)

target_link_libraries(
    plcp-decomp2
    tudocomp
    tudocomp_algorithms
    glog
    sdsl
	pthread
	gomp
)

cotire(plcp-decomp2)
    
add_custom_command(TARGET plcp-decomp2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/plcp-decomp2 ${CMAKE_BINARY_DIR}/plcp-decomp2)
