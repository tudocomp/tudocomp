add_library(
    sdsl_extension
    INTERFACE
)
target_link_libraries(
    sdsl_extension
    INTERFACE
    sdsl
    divsufsort
    divsufsort64
    ${GLOG_LIBRARIES}
)
target_include_directories(
    sdsl_extension
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
