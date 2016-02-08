add_library(
    esacomp_impl

    max_lcp_sorted_suffix_list.cpp
    max_lcp_heap.cpp
    code0.cpp
    code1.cpp
    code2.cpp
)

target_link_libraries(
    esacomp_impl

    esacomp
    tudocomp
    esacomp_suffix_array_compressor
    glog
)

target_include_directories(esacomp_impl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
