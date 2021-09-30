macro(git_submodule_subdirectory dep_name)
execute_process(COMMAND git submodule update --init -- ${dep_name}
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(${dep_name})
endmacro()

# NB: Could add ALL specifier to ensure its up-to-date on each build
# Maybe add VERBATIM to output?
add_custom_target(
    update_submodules
    COMMAND git submodule update --recursive --remote
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Updating GIT submodules..."
)
