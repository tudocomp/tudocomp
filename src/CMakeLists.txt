
# Main TuDoComp library and command line interface
add_subdirectory(tudocomp)
add_subdirectory(tudocomp_driver)

# Compressor that uses lz77-like substitution rules
# with separate compression and encoding steps
add_subdirectory(lz77rule)

# Compressor that uses lz78-like dictionaries
# with separate compression and encoding steps
add_subdirectory(lz78rule)

# Compressor that uses lzw-like dictionaries
# with separate compression and encoding steps
add_subdirectory(lzwrule)

# sdsl extension features
add_subdirectory(sdsl_extension)

# Adapter library for lz77rule compression algorithms working with suffix arrays
add_subdirectory(suffix_array_compressor)

# The lz77rule "Dummy" library. Does no-op compression and encoding, and is
# used as example and for comparison.
add_subdirectory(dummy)

# The lz77rule Esacomp library.
add_subdirectory(esacomp)

# The lz77rule LZ implementations
add_subdirectory(lz_compressor)

# CLI tool for comparing different compression tools
add_subdirectory(compare_tool)
