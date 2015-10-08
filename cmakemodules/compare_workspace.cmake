# custom target: compare_workspace
# > dir compare_workspace
# > run.sh script
#   > make tudocomp_driver
#   > make compress_tool
#   > compress_tool ARGS

add_custom_command(
   OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/compare_workspace/run.sh
   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/etc/compare_run.sh ${CMAKE_CURRENT_BINARY_DIR}/compare_workspace/run.sh
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/etc/compare_run.sh
)
add_custom_command(
   OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/compare_workspace/config.toml
   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/etc/compare_config.toml ${CMAKE_CURRENT_BINARY_DIR}/compare_workspace/config.toml
   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/etc/compare_config.toml
)
add_custom_target(compare_workspace
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/compare_workspace
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/compare_workspace/run.sh
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/compare_workspace/config.toml
)
