add_library(
    tudocomp

    lz78_impl.cpp
    lzw_impl.cpp
)

target_link_libraries(
    tudocomp

    glog
    gflags
    sdsl
)

