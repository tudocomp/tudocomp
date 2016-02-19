add_library(
    esacomp
    INTERFACE
)
target_link_libraries(
    esacomp
    INTERFACE
    tudocomp
    sdsl_extension
    glog
)
target_include_directories(
    esacomp
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)
