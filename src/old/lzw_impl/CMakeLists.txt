add_library(
    lzw_impl

    lzw.cpp
)

target_link_libraries(
    lzw_impl

    tudocomp
    esacomp_suffix_array_compressor
    lzw
    lz78 # Reuses the dictionary trie
    glog
)

target_include_directories(lzw_impl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
