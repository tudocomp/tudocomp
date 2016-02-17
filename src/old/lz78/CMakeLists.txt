add_library(
    lz78
    INTERFACE
)
target_link_libraries(
    lz78
    INTERFACE
    tudocomp
    sdsl_extension
    glog
)
target_include_directories(
    lz78
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
