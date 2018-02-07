macro(git_submodule_subdirectory dep_name)
execute_process(COMMAND git submodule update --init -- ${dep_name}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(${dep_name})
endmacro()
