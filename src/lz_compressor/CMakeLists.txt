add_library(
    lz_compressor

    lz_compressor.cpp
    lz78.cpp
)

target_link_libraries(
    lz_compressor

    tudocomp
    suffix_array_compressor
    ${GLOG_LIBRARIES}
)

target_include_directories(lz_compressor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
