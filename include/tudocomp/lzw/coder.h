#ifndef LZW_CODER_H
#define LZW_CODER_H

#include <tudocomp/env.h>
#include <tudocomp/io.h>

#include <tudocomp/lzw/factor.h>

namespace lzw {

using namespace tudocomp;

/// Interface for a coder from LZW-like substitution rules.
class LzwRuleCoder {
public:
    Env& env;

    /// Class needs to be constructed with an `Env&` argument.
    inline LzwRuleCoder() = delete;

    /// Construct the class with an environment.
    inline LzwRuleCoder(Env& env_): env(env_) {}

    /// Encode a list or LzwEntries and the input text.
    ///
    /// \param rules The list of substitution rules
    /// \param out `ostream` where the encoded output will be written to.
    virtual void code(LzwEntries&& rules, Output& out) = 0;

    /// Decode and decompress `inp` into `out`.
    ///
    /// This method expects `inp` to be encoded with the same encoding
    /// that the `code()` method emits.
    ///
    /// \param inp The input stream.
    /// \param out The output stream.
    virtual void decode(Input& inp, Output& out) = 0;
};

}

#endif
