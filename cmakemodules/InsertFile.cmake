# Simple CMake script to read the contents of one file and insert it into another
file(READ ${INPUT} CONTENT)
configure_file(${OUTPUT} ${OUTPUT})
