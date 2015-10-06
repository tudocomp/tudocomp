
add_custom_target(datasets
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/get_examples.sh"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Don't build this automatically
set_target_properties(datasets PROPERTIES EXCLUDE_FROM_ALL true)
