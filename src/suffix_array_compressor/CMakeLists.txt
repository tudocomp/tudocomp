add_library(
    suffix_array_compressor

    sa_compressor.cpp
)

target_link_libraries(
    suffix_array_compressor

    tudocomp
    lz77rule
    sdsl
    glog
)

target_include_directories(suffix_array_compressor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
