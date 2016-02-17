add_library(
    lzw
    INTERFACE
)
target_link_libraries(
    lzw
    INTERFACE
    tudocomp
    sdsl_extension
    glog
)
target_include_directories(
    lzw
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
