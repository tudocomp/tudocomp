add_library(
    esacomp_suffix_array_compressor

    sa_compressor.cpp
)

target_link_libraries(
    esacomp_suffix_array_compressor

    tudocomp
    esacomp
    sdsl
    glog
)

target_include_directories(esacomp_suffix_array_compressor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
