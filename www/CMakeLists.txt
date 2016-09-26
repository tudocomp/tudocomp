
set(WWW_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/html)

# www - Copies the contents of "www" to the build tree
add_custom_target(www
    COMMENT "Copying website files" VERBATIM
    COMMAND ${CMAKE_COMMAND} -E make_directory ${WWW_OUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR} ${WWW_OUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E remove ${WWW_OUT_DIR}/CMakeLists.txt
)

# website - Builds the website
add_custom_target(website
    COMMENT "Generating website" VERBATIM
    DEPENDS www docs-main-html docs-doxygen
)
