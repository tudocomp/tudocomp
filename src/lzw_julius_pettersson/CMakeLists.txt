add_library(
    lzw_jp

    lzw_v6.cpp
)

target_include_directories(lzw_jp
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
