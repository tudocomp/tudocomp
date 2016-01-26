add_library(
    lz_compressor

    lz_compressor.cpp
    lz78.cpp
    lz77.cpp
    lzw.cpp
)

target_link_libraries(
    lz_compressor

    tudocomp
    suffix_array_compressor
    lz78rule
    lz77rule
    lzwrule
    glog
)

target_include_directories(lz_compressor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
