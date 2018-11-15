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

cotire(tudocomp_stat)
