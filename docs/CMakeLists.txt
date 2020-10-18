# docs-doxygen target - builds the Doxygen documentation
find_package(Doxygen)

if(DOXYGEN_FOUND)
    # modules
    list(APPEND TDC_DOXYGEN_INPUT
        ${CMAKE_SOURCE_DIR}/README.md
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/test/test/util.hpp
        ${CMAKE_SOURCE_DIR}/submodules/bit_span/include
        ${CMAKE_SOURCE_DIR}/submodules/tudostats/include
        ${CMAKE_SOURCE_DIR}/submodules/compact_sparse_hash/include
        ${CMAKE_CURRENT_SOURCE_DIR}/content/start.md # fixed order
        ${CMAKE_CURRENT_SOURCE_DIR}/content/docs.md  # fixed order
        ${CMAKE_CURRENT_SOURCE_DIR}/content/impl.md  # fixed order
        ${CMAKE_CURRENT_SOURCE_DIR}/content
    )

    string (REPLACE ";" " " TDC_DOXYGEN_INPUT "${TDC_DOXYGEN_INPUT}")
    message("TDC_DOXYGEN_INPUT: ${TDC_DOXYGEN_INPUT}")

    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

    add_custom_target(docs
        #invoke doxygen
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating doxygen documentation"
    )
endif()
