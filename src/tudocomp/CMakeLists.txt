add_library(
    tudocomp

    tudocomp.cpp
)
target_link_libraries(
    tudocomp

    sdsl
    divsufsort
    divsufsort64
    custom_glog
)
target_include_directories(
    tudocomp

    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
