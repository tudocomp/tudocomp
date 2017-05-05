#pragma once

namespace tdc{namespace io {
    /// \brief Represents a file path.
    ///
    /// Pass a Path instance to the respective constructor in order to
    /// create an input from the file pointed to by the path.
    struct Path {
        explicit inline Path(const std::string& p):
            path(p) {}

        /// The path string.
        std::string path;
    };
}}
