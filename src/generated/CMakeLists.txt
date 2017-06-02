set(GENERATE_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated_source)
file(MAKE_DIRECTORY ${GENERATE_DIR})

set(GENREGSCRIPT ${CMAKE_SOURCE_DIR}/etc/genregistry.py)
set(GENREGCOMMAND
    ${GENREGSCRIPT}
    ${CMAKE_SOURCE_DIR}/etc/registry_config.py
    ${CMAKE_BINARY_DIR}/include/tudocomp/config.h
    ${GENERATE_DIR}
    --group 20
)

execute_process(
    COMMAND ${GENREGCOMMAND} --print_deps
    OUTPUT_VARIABLE OUTPUTS
    RESULT_VARIABLE RETURN_VALUE
)
if (NOT RETURN_VALUE EQUAL 0)
    message(FATAL_ERROR "Error in genregistry.py")
endif()

add_custom_command(
    COMMAND ${GENREGCOMMAND} --generate_files
    DEPENDS ${GENREGSCRIPT}
    OUTPUT ${OUTPUTS}
    COMMENT "Generating the registry..."
)

add_library(
    tudocomp_algorithms
    ${OUTPUTS}
)
target_link_libraries(
    tudocomp_algorithms

    tudocomp_stat
    tudocomp
    glog
    sdsl
)

cotire(tudocomp_algorithms)

configure_file(${GENREGSCRIPT} ${GENERATE_DIR}/trash COPYONLY)
