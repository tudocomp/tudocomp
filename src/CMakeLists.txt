# Standalone command line tools

    # TuDoComp command line interface
    add_subdirectory(tudocomp_driver)

    # CLI tool for comparing different compression tools
    add_subdirectory(compare_tool)

# Standalone support libraries

    # sdsl extension features
    add_subdirectory(sdsl_extension)

# TuDoComp framework

    # Main TuDoComp library
    add_subdirectory(tudocomp)

    # Esacomp compressor.
    # Based around substitution rules of the form
    # (absolute_pos, absolute_target, len)
    # interleaved in plain text.
    add_subdirectory(esacomp)

        # Actual esacomp implementation from the original BA
        add_subdirectory(esacomp_impl)

        # Replacement rules based on a lz factorization from a suffix array
        add_subdirectory(esacomp_lz_factor)

        # "Dummy" compressor and encoder that behave as no-ops
        add_subdirectory(esacomp_dummy)

        # Adapter interface for preparing the input as a suffix array.
        # Used by most esacomp_* impls.
        add_subdirectory(esacomp_suffix_array_compressor)

    # Lz77 classic.
    # Based around a stream of encoding rules in the form of
    # (back_relative_pos, len, next_char).
    #add_subdirectory(lz77_classic)

    # Lz77 ss.
    # Based around substitution rules of the form
    # (back_relative_pos, len)
    # interleaved in plain text.
    #add_subdirectory(lz77_ss)

    # Lz78.
    # Based around a stream of encoding rules in the form of
    # (prev_dict_idx, next_char).
    add_subdirectory(lz78)
        # Basic debug and bit-compressed encoders.
        add_subdirectory(lz78_impl)

    # Lzw.
    # Based around a stream of encoding rules in the form of
    # (prev_dict_idx).
    add_subdirectory(lzw)
        # Basic debug and bit-compressed encoders.
        add_subdirectory(lzw_impl)
