add_library(
    esacomp_dummy

    dummy_compressor.cpp
    dummy_encoder.cpp
)

target_link_libraries(
    esacomp_dummy

    tudocomp
    esacomp
    glog
)

target_include_directories(esacomp_dummy PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
