add_library(
    lzwrule
    INTERFACE
)
target_link_libraries(
    lzwrule
    INTERFACE
    tudocomp
    sdsl_extension
    glog
)
target_include_directories(
    lzwrule
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
