
set(WWW_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/html)

# www - Copies the contents of "www" to the build tree
add_custom_target(www
    COMMENT "Copying website files" VERBATIM
    COMMAND ${CMAKE_COMMAND} -E make_directory ${WWW_OUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR} ${WWW_OUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E remove ${WWW_OUT_DIR}/CMakeLists.txt
)

set(DOCS_BINARY_DIR ${CMAKE_BINARY_DIR}/docs)
set(WWW_DOCUMENTATION_DIR ${WWW_OUT_DIR}/documentation)
set(WWW_DOXYGEN_DIR ${WWW_OUT_DIR}/doxygen)

# website - Builds the website
add_custom_target(website
    COMMENT "Generating website" VERBATIM
    DEPENDS www docs-main-html docs-doxygen
    # Copy documentation
    COMMAND ${CMAKE_COMMAND} -E make_directory ${WWW_DOCUMENTATION_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${DOCS_BINARY_DIR}/main/html ${WWW_DOCUMENTATION_DIR}
    # Copy Doxygen API reference
    COMMAND ${CMAKE_COMMAND} -E make_directory ${WWW_DOXYGEN_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${DOCS_BINARY_DIR}/doxygen/html ${WWW_DOXYGEN_DIR}
)
