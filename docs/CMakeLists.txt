# docs-main target - builds the main documentation
find_package(LATEX COMPONENTS PDFLATEX)
find_package(Pandoc)

if(PANDOC_FOUND)

    include(${CMAKE_CURRENT_SOURCE_DIR}/DocumentationVars.txt)

    set(MAIN_LATEX_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/main/latex)
    set(MAIN_HTML_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/main/html)

    set(PANDOC_COMMAND ${PANDOC_BIN} -f markdown)

    file(MAKE_DIRECTORY ${MAIN_LATEX_OUT_DIR})
    file(MAKE_DIRECTORY ${MAIN_HTML_OUT_DIR})

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Documentation.md ${CMAKE_CURRENT_BINARY_DIR}/Documentation.processed.md @ONLY)

    add_custom_target(docs-main-html

        # convert markdown doc to html
        COMMAND GHCRTS=-V0 ${PANDOC_COMMAND} -t html --toc
            --template=${CMAKE_CURRENT_SOURCE_DIR}/pandoc_template.html
            -o ${CMAKE_CURRENT_BINARY_DIR}/_content.html
            ${CMAKE_CURRENT_BINARY_DIR}/Documentation.processed.md

        # copy html framework
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/html ${MAIN_HTML_OUT_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/media ${MAIN_HTML_OUT_DIR}/media

        # insert contents into HTML page
        COMMAND ${CMAKE_COMMAND} -DOUTPUT=${MAIN_HTML_OUT_DIR}/index.html
                                 -DINPUT=${CMAKE_CURRENT_BINARY_DIR}/_content.html
                                 -P ${CMAKE_SOURCE_DIR}/cmakemodules/InsertFile.cmake

        WORKING_DIRECTORY ${MAIN_HTML_OUT_DIR}
        COMMENT "Generating main html documentation" VERBATIM
    )

    if(LATEX_PDFLATEX_FOUND)

        add_custom_target(docs-main-pdf
            # convert markdown doc to tex
            COMMAND ${PANDOC_COMMAND} -t latex
                -o ${MAIN_LATEX_OUT_DIR}/_content.tex
                ${CMAKE_CURRENT_BINARY_DIR}/Documentation.processed.md

            # copy tex framework
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/latex ${MAIN_LATEX_OUT_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/media ${MAIN_LATEX_OUT_DIR}/media

            # build pdf
            COMMAND ${PDFLATEX_COMPILER} -synctex=1 -interaction=nonstopmode main.tex #pass 1
            COMMAND ${PDFLATEX_COMPILER} -synctex=1 -interaction=nonstopmode main.tex #pass 2
            # move it to output directory
            COMMAND ${CMAKE_COMMAND} -E rename ${MAIN_LATEX_OUT_DIR}/main.pdf ${CMAKE_CURRENT_BINARY_DIR}/Documentation.pdf

            #
            WORKING_DIRECTORY ${MAIN_LATEX_OUT_DIR}
            COMMENT "Generating main pdf documentation" VERBATIM
        )

        add_custom_target(docs-main
            COMMENT "Generating main documentation" VERBATIM
            DEPENDS docs-main-html docs-main-pdf
        )

    else(LATEX_PDFLATEX_FOUND)

        add_custom_target(docs-main
            COMMENT "Generating main documentation (PDF unavailable)" VERBATIM
            DEPENDS docs-main-html
        )

    endif(LATEX_PDFLATEX_FOUND)

endif(PANDOC_FOUND)

# docs-doxygen target - builds the Doxygen documentation
find_package(Doxygen)
if(DOXYGEN_FOUND)

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(docs-doxygen
        #invoke doxygen
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        # copy custom Doxygen skin
        #COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/doxygen-skin #${CMAKE_CURRENT_BINARY_DIR}/doxygen/html
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating doxygen documentation"
    )
endif(DOXYGEN_FOUND)

# docs target - invokes docs-main and docs-doxygen
if(DOXYGEN_FOUND AND PANDOC_FOUND)
add_custom_target(docs DEPENDS docs-main docs-doxygen)

endif(DOXYGEN_FOUND AND PANDOC_FOUND)

