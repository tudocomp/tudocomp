# Create executable for every *.cpp file
add_custom_target(examples)

FILE(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)
FOREACH(child ${children})
    IF(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child})
        add_subdirectory(${child})
    ENDIF()
ENDFOREACH()

add_executable(
    coder_stats_example

    coder_stats.cpp
)

target_link_libraries(
    coder_stats_example

    ${TDC_DEPENDS}
    tudocomp
)

add_dependencies(examples coder_stats_example)

