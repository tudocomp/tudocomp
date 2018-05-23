# Create executable for every *.cpp file
add_custom_target(examples)

FILE(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)
FOREACH(child ${children})
    IF(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child})
        add_subdirectory(${child})
    ENDIF()
ENDFOREACH()

