add_library(
    lz78_impl

    lz78.cpp
)

target_link_libraries(
    lz78_impl

    tudocomp
    esacomp_suffix_array_compressor
    lz78
    glog
)

target_include_directories(lz78_impl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
