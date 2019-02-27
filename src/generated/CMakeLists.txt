set(GENERATE_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated_source)
file(MAKE_DIRECTORY ${GENERATE_DIR})

set(GENREGCONFIG ${TDC_REGISTRY})
set(GENREGSCRIPT ${CMAKE_SOURCE_DIR}/etc/genregistry.py)
set(GENREGCOMMAND
    ${GENREGSCRIPT}
    ${GENREGCONFIG}
    ${CMAKE_BINARY_DIR}/include/tudocomp/config.h
    ${GENERATE_DIR}
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
    DEPENDS ${GENREGSCRIPT} ${GENREGCONFIG}
    OUTPUT ${OUTPUTS}
    COMMENT "Generating the registry..."
)

add_library(
    tudocomp_algorithms
    ${OUTPUTS}
)
target_link_libraries(
    tudocomp_algorithms

    ${TDC_DEPENDS}
    tudocomp_stat
    tudocomp
)

cotire(tudocomp_algorithms)

configure_file(${GENREGSCRIPT} ${GENERATE_DIR}/trash1 COPYONLY)
configure_file(${GENREGCONFIG} ${GENERATE_DIR}/trash2 COPYONLY)
