add_library(
    esacomp_lz_factor

    lz_compressor.cpp
)

target_link_libraries(
    esacomp_lz_factor

    tudocomp
    esacomp
    esacomp_suffix_array_compressor
    glog
)

target_include_directories(esacomp_lz_factor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
