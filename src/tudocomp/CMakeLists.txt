add_library(
    tudocomp

    meta/Registry.cpp
)

target_link_libraries(
    tudocomp

    tudocomp_stat
    glog
    sdsl
    bit_span
    compact_sparse_hash
)

cotire(tudocomp)
