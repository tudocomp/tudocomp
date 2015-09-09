
# Main TuDoComp library and command line interface
add_subdirectory(tudocomp)
add_subdirectory(tudocomp_driver)

# Adapter library for compression algorithms working with suffix arrays
add_subdirectory(suffix_array_compressor)

# "Dummy" library. Does no-op compression and encoding, and is
# used as example and for comparision
add_subdirectory(dummy)

# Esacomp library.
add_subdirectory(esacomp)

# LZ implementations
add_subdirectory(lz_compressor)
