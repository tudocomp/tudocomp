add_library(
    esacomp

    max_lcp_sorted_suffix_list.cpp
    max_lcp_heap.cpp
    code0.cpp
    code1.cpp
    code2.cpp
)

target_link_libraries(
    esacomp

    esacomprule
    tudocomp
    suffix_array_compressor
    glog
)

target_include_directories(esacomp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
