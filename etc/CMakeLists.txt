macro(dataset_target dataset_name dataset_config)
    add_custom_target(${dataset_name}
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/get_datasets.py ${CMAKE_CURRENT_SOURCE_DIR}/datasets ${CMAKE_CURRENT_SOURCE_DIR}/${dataset_config} --delete_original
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    set_target_properties(${dataset_name} PROPERTIES EXCLUDE_FROM_ALL true)
endmacro(dataset_target)

dataset_target(datasets       datasets_config.py)
dataset_target(large_datasets large_datasets_config.py)
