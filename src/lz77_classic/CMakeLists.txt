add_library(
    lz78rule
    INTERFACE
)
target_link_libraries(
    lz78rule
    INTERFACE
    tudocomp
    sdsl_extension
    glog
)
target_include_directories(
    lz78rule
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
