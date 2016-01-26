add_library(
    dummy

    dummy_compressor.cpp
    dummy_encoder.cpp
)

target_link_libraries(
    dummy

    tudocomp
    lz77rule
    glog
)

target_include_directories(dummy PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
