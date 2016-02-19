add_executable(
    tudocomp

    #util/decode_buffer.cpp
    lz78_impl.cpp
    lzw_impl.cpp
    tudocomp_algorithms.cpp
    tudocomp.cpp
)

target_link_libraries(
    tudocomp
    glog
    sdsl
    boost_program_options
)
