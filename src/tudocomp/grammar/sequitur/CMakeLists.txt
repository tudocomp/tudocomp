add_library(
    sequitur

    arith.c
    bitio.c
    classes.cpp
    compress.cpp
    getopt.c
    #sequitur.cpp
    sequitur_vars.cpp
    stats.c
)


target_link_libraries(
    sequitur

    ${TDC_DEPENDS} 
)
