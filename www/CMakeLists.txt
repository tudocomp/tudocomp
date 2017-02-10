
set(WWW_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/html)

# www - Copies the contents of "www" to the build tree
add_custom_target(www
    COMMENT "Copying website files" VERBATIM
    COMMAND ${CMAKE_COMMAND} -E make_directory ${WWW_OUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR} ${WWW_OUT_DIR}
    # "exclude" these
    COMMAND ${CMAKE_COMMAND} -E remove ${WWW_OUT_DIR}/CMakeLists.txt
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${WWW_OUT_DIR}/pages
)

find_package(Pandoc)

if(PANDOC_FOUND)

    # Builds the Markdown pages in the "pages" directory
    file(GLOB PAGES "${CMAKE_CURRENT_SOURCE_DIR}/pages/*.md")
    foreach(PAGE ${PAGES})
        get_filename_component(PAGENAME ${PAGE} NAME_WE)
        add_custom_command(TARGET www POST_BUILD
            COMMAND ${PANDOC_BIN} -f markdown -t html
                --template=${CMAKE_CURRENT_SOURCE_DIR}/pages/pandoc_template.html
                -o ${WWW_OUT_DIR}/${PAGENAME}.html
                ${PAGE}
            WORKING_DIRECTORY ${WWW_OUT_DIR}
        )
    endforeach()

endif(PANDOC_FOUND)

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
