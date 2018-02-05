add_library(
    tudocomp_stat

    malloc.cpp
    StatPhase.cpp
    PhaseData.cpp
)

target_link_libraries(
    tudocomp_stat
    tudocomp
)

cotire(tudocomp_stat)
