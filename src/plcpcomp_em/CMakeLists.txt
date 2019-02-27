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

    ${TDC_DEPENDS}
    tudocomp
    tudocomp_algorithms
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

    ${TDC_DEPENDS}
    tudocomp
    tudocomp_algorithms
    pthread
    gomp
)

cotire(plcp-decomp)

add_custom_command(TARGET plcp-decomp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/plcp-decomp ${CMAKE_BINARY_DIR}/plcp-decomp)
