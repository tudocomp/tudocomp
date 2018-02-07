execute_process(COMMAND git submodule update --init -- datasets
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(datasets)
