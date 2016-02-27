#ifndef _INCLUDED_COMPRESSOR_HPP_
#define _INCLUDED_COMPRESSOR_HPP_

#include <tudocomp/env.h>
#include <tudocomp/io.h>

namespace tudocomp {

/// Interface for a general compressor.
class Compressor {
protected:
    Env* m_env;

public:
    /// Class needs to be constructed with an `Env&` argument.
    inline Compressor() = delete;

    /// Construct the class with an environment.
    inline Compressor(Env& env): m_env(&env) {}

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
