add_library(
    esacomprule
    INTERFACE
)
target_link_libraries(
    esacomprule
    INTERFACE
    tudocomp
    sdsl_extension
    glog
)
target_include_directories(
    esacomprule
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include
)
