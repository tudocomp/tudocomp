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

if(STXXL_FOUND)
    target_link_libraries(tudocomp ${STXXL_LIBRARIES})
endif(STXXL_FOUND)

cotire(tudocomp)
