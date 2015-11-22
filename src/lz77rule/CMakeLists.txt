add_library(
    lz77rule
    INTERFACE
)
target_link_libraries(
    lz77rule
    INTERFACE
    tudocomp
    ${GLOG_LIBRARIES}
)
target_include_directories(
    lz77rule
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
