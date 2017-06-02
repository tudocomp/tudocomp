file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/include/tudocomp)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/include/tudocomp/config.h.in ${CMAKE_BINARY_DIR}/include/tudocomp/config.h)

add_custom_target(
    generate_version
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/etc/genversion.sh ${CMAKE_BINARY_DIR}/include/tudocomp/version.hpp ${VERSION}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/etc/genversion.sh
)

include_directories(${CMAKE_BINARY_DIR}/include)
