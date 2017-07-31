macro(dataset_target dataset_name dataset_parent_target)
    add_custom_target(datasets_${dataset_name}
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/get_datasets ${CMAKE_CURRENT_SOURCE_DIR}/datasets ${CMAKE_CURRENT_SOURCE_DIR}/dataset_${dataset_name}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    add_dependencies(${dataset_parent_target} datasets_${dataset_name})
endmacro(dataset_target)

add_custom_target(datasets)
set_target_properties(datasets PROPERTIES EXCLUDE_FROM_ALL true)
dataset_target(pizza_chili datasets)
dataset_target(pizza_chili_repetitive datasets)
dataset_target(tagme datasets)
dataset_target(tagmynews datasets)
#dataset_target(hashtag datasets)
dataset_target(wiki datasets)
dataset_target(commoncrawl datasets)

add_custom_target(large_datasets)
set_target_properties(large_datasets PROPERTIES EXCLUDE_FROM_ALL true)
dataset_target(large large_datasets)
