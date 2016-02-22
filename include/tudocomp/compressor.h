#ifndef COMPRESSFRAMEWORK_H
#define COMPRESSFRAMEWORK_H

#include <tudocomp/env.h>
#include <tudocomp/io.h>

namespace tudocomp {

/// Interface for a general compressor.
struct Compressor {
    // TODO: Maybe replace by pointer to make this class properly copyable?
    Env& env;

    /// Class needs to be constructed with an `Env&` argument.
    inline Compressor() = delete;

    /// Construct the class with an environment.
    inline Compressor(Env& env_): env(env_) {}

    /// Compress `inp` into `out`.
    ///
    /// \param inp The input stream.
    /// \param out The output stream.
    virtual void compress(Input& input, Output& output) = 0;

    /// Decompress `inp` into `out`.
    ///
    /// \param inp The input stream.
    /// \param out The output stream.
    virtual void decompress(Input& input, Output& output) = 0;
};

}

#endif
