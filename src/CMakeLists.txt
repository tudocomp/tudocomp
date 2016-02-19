add_executable(
    tudocomp-dummy

    util/decode_buffer.cpp
    dummy.cpp
)

target_link_libraries(
    tudocomp-dummy
    glog
    sdsl
)
