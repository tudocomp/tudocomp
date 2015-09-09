add_library(
    tudocomp

    tudocomp.cpp
    rule.cpp
)

target_include_directories(
    tudocomp

    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
