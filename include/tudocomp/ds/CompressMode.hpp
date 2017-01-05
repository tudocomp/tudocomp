#pragma once

namespace tdc {

    /// Defines when data structures are compressed.
    enum CompressMode {
        /// Data structures aren't compressed
        /// (fastest, but highest memory usage).
        none = 0,

        /// Data structures are compressed after they have been constructed
        /// (fast construction, but possibly high memory peak).
        delayed = 1,

        /// Data structures are constructed directly in compressed space
        /// (slower construction, but smaller memory usage).
        direct = 2,
    };

}
