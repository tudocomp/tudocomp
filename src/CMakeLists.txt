add_executable(
    tudocomp-dummy

    util/decode_buffer.cpp
    lz78_impl.cpp
    dummy.cpp
)

target_link_libraries(
    tudocomp-dummy
    glog
    sdsl
)
