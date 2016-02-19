#ifndef LZ78_CODER_H
#define LZ78_CODER_H

#include <tudocomp/env.h>
#include <tudocomp/io.h>

#include <tudocomp/lz78/factors.h>

namespace lz78 {
    
using namespace tudocomp;

/// Interface for a coder from LZ77-like substitution rules.
///
/// This takes a list of Entries and the input text, and outputs
/// an encoded form of them to a `ostream`. Also provided is a decoder,
/// that takes such an encoded stream and outputs the fully
/// decoded and decompressed original text.
class Lz78RuleCoder {
public:
    const Env& env;

    /// Class needs to be constructed with an `Env&` argument.
    inline Lz78RuleCoder() = delete;

    /// Construct the class with an environment.
    inline Lz78RuleCoder(Env& env_): env(env_) {}

    /// Encode a list or Entries and the input text.
    ///
    /// \param rules The list of substitution rules
    /// \param input The input text
    /// \param out `ostream` where the encoded output will be written to.
    virtual void code(Entries&& rules, Output& out) = 0;

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
