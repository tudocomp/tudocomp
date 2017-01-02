add_custom_target(datasets)

macro(dataset_target dataset_name)
    add_custom_target(datasets_${dataset_name}
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/get_datasets ${CMAKE_CURRENT_SOURCE_DIR}/datasets ${CMAKE_CURRENT_SOURCE_DIR}/dataset_${dataset_name}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    add_dependencies(datasets datasets_${dataset_name})
endmacro(dataset_target)

dataset_target(pizza_chili)
dataset_target(tagme)
dataset_target(tagmynews)
dataset_target(hashtag)
dataset_target(wiki)

# Don't build this automatically
set_target_properties(datasets PROPERTIES EXCLUDE_FROM_ALL true)
