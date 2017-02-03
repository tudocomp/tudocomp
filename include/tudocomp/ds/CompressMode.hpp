#pragma once

namespace tdc {

    /// Defines when data structures are bit-compressed.
    enum CompressMode {
        /// Data structures are not bit-compressed at all
        /// (fastest, but highest memory usage).
        plain = 0,

        /// Data structures are bit-compressed after they have been constructed
        /// (fast construction, but possibly high memory peak).
        delayed = 1,

        /// Data structures are constructed directly in bit-compressed space
        /// (slower construction, but smaller memory usage).
        compressed = 2,

        /// Special mode that will cause no bit-compression at all during
        /// construction, internal use in TextDS only
        coherent_delayed = 254,

        /// Automatically select compress mode, internal use in TextDS only
        select = 255,
    };

    inline CompressMode cm_select(CompressMode in, CompressMode sel) {
        return in == CompressMode::select ? sel : in;
    }

}
