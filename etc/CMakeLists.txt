add_custom_target(datasets
    DEPENDS datasets_pizza_chili
)

add_custom_target(datasets_pizza_chili
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/get_datasets_pizza_chili ${CMAKE_CURRENT_SOURCE_DIR}/datasets
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Don't build this automatically
set_target_properties(datasets PROPERTIES EXCLUDE_FROM_ALL true)

