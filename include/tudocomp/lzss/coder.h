#ifndef LZSS_CODER_H
#define LZSS_CODER_H

#include <tudocomp/io.h>
#include <tudocomp/util/env.h>
#include <tudocomp/lzss/factors.h>

namespace lz77rule {
    
using namespace tudocomp;


/// Interface for a coder from LZ77-like substitution rules.
///
/// This takes a list of Rules and the input text, and outputs
/// an encoded form of them to a `ostream`. Also provided is a decoder,
/// that takes such an encoded stream and outputs the fully
/// decoded and decompressed original text.
class Lz77RuleCoder {
public:
    Env& env;

    /// Class needs to be constructed with an `Env&` argument.
    inline Lz77RuleCoder() = delete;

    /// Construct the class with an environment.
    inline Lz77RuleCoder(Env& env_): env(env_) {}

    /// Encode a list or Rules and the input text.
    ///
    /// \param rules The list of substitution rules
    /// \param input The input text
    /// \param out `ostream` where the encoded output will be written to.
    virtual void code(Rules&& rules, Input& input, Output& output) = 0;

    /// Decode and decompress `inp` into `out`.
    ///
    /// This method expects `inp` to be encoded with the same encoding
    /// that the `code()` method emits.
    ///
    /// \param inp The input stream.
    /// \param out The output stream.
    virtual void decode(Input& inp, Output& out) = 0;

    /// Return the expected minimum encoded
    /// length in bytes of a single rule if encoded with this encoder.
    ///
    /// This can be used by compressors to directly filter
    /// out rules that would not be beneficial in the encoded output.
    ///
    /// \param input_size The length of the input in bytes
    virtual size_t min_encoded_rule_length(size_t input_size = SIZE_MAX) = 0;
};
    
}

#endif
