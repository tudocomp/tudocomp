add_library(
    tudocomp

    INTERFACE
)

target_link_libraries(
    tudocomp

    INTERFACE

    tudostat
    glog
    gflags
    sdsl
)

