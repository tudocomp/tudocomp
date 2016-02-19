#ifndef COMPRESSFRAMEWORK_H
#define COMPRESSFRAMEWORK_H

#include <tudocomp/io.h>
#include <tudocomp/util.h>
#include <tudocomp/util/env.h>

namespace tudocomp {

/// Handle of the input data stream.
using Input = input::Input;

/// Handle of the output data stream.
using Output = output::Output;

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
