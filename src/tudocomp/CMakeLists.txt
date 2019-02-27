add_library(
    tudocomp

    meta/Registry.cpp
    meta/ast/Parser.cpp
    util/STXXLStatExtension.cpp
)

target_link_libraries(
    tudocomp

    ${TDC_DEPENDS}
    tudocomp_stat
    bit_span
    compact_sparse_hash
)

if(SDSL_FOUND)
    target_link_libraries(tudocomp ${SDSL_LIBRARIES})
endif(SDSL_FOUND)

if(STXXL_FOUND)
    target_link_libraries(tudocomp ${STXXL_LIBRARIES})
endif(STXXL_FOUND)

cotire(tudocomp)
