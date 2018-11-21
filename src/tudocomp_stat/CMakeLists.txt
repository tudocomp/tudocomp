add_library(
    tudocomp_stat

    malloc.cpp
    StatPhase.cpp
    StatPhaseStxxl.cpp
    PhaseData.cpp
)

target_link_libraries(
    tudocomp_stat
    tudocomp
)

if(STXXL_FOUND)
    target_link_libraries(tudocomp_stat ${STXXL_LIBRARIES})
endif(STXXL_FOUND)

cotire(tudocomp_stat)
