add_library(
    tudocomp_stat

    malloc.cpp
    Stat.cpp
)

target_link_libraries(
    tudocomp_stat
    tudocomp
)

cotire(tudocomp_stat)
