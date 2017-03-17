add_library(
    tudocomp

    INTERFACE
)

target_link_libraries(
    tudocomp

    INTERFACE

    tudocomp_stat
    glog
    sdsl
)

