add_library(
    malloc_count

    malloc_count.cpp
    tudostat_module.cpp
)

target_link_libraries(
    malloc_count

    tudocomp
)
