CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/include/tudocomp/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/tudocomp/config.h)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/tudocomp)

add_custom_command(
    PRE_BUILD
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tudocomp_algorithms.cpp ${CMAKE_CURRENT_BINARY_DIR}/include/tudocomp/tudocomp.hpp
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/etc/genregistry.py ${CMAKE_CURRENT_BINARY_DIR}/tudocomp_algorithms.cpp ${CMAKE_CURRENT_BINARY_DIR}/include/tudocomp/tudocomp.hpp ${CMAKE_CURRENT_SOURCE_DIR}/include/tudocomp/config.h
	COMMENT "Generating tudocomp_algorithms.cpp and tudocomp.hpp..."
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/etc/genregistry.py ${CMAKE_CURRENT_SOURCE_DIR}/include/tudocomp/config.h
)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
