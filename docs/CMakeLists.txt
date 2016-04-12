# docs-main target - builds the main documentation
find_package(Pandoc)
if(PANDOC_FOUND)
add_custom_target(docs-main
# convert markdown doc to pdf via latex
${PANDOC_BIN} -f markdown -t latex -o ${CMAKE_CURRENT_BINARY_DIR}/Documentation.pdf --template=${CMAKE_CURRENT_SOURCE_DIR}/latex/template.tex ${CMAKE_CURRENT_SOURCE_DIR}/Documentation.md
WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/latex
COMMENT "Generating main documentation" VERBATIM
)
endif(PANDOC_FOUND)

# docs-doxygen target - builds the Doxygen documentation
find_package(Doxygen)
if(DOXYGEN_FOUND)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
add_custom_target(docs-doxygen
#invoke doxygen
${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
# make doxygen PDF
COMMAND cd ${CMAKE_CURRENT_BINARY_DIR}/doxygen/latex && make
# move it to output directory
COMMAND mv ${CMAKE_CURRENT_BINARY_DIR}/doxygen/latex/refman.pdf ${CMAKE_CURRENT_BINARY_DIR}/Doxygen.pdf
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
COMMENT "Generating doxygen documentation"
)
endif(DOXYGEN_FOUND)

# docs target - invokes docs-main and docs-doxygen
if(DOXYGEN_FOUND AND PANDOC_FOUND)
add_custom_target(docs DEPENDS docs-main docs-doxygen)
endif(DOXYGEN_FOUND AND PANDOC_FOUND)
