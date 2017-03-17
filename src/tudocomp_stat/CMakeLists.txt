add_library(
    tudocomp_stat

    Stat.cpp
    malloc.cpp
)

target_link_libraries(
    tudocomp_stat

    tudocomp
)

cotire(tudocomp_stat)

