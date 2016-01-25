add_library(
    lz77rule
    INTERFACE
)
target_link_libraries(
    lz77rule
    INTERFACE
    tudocomp
    sdsl_extension
    custom_glog
)
target_include_directories(
    lz77rule
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
