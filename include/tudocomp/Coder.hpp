#ifndef _INCLUDED_EXAMPLE_CODER_HPP_
#define _INCLUDED_EXAMPLE_CODER_HPP_

#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>

namespace tudocomp {

/// Example interface of a coder.
///
/// A Coder implementation needs to implement all listed
/// methods and constructors.
template<typename F>
class ExampleCoder {
public:
    /// Construct a Coder that writes to a given Output
    ExampleCoder(Env& env, Output& out) {
    }

    /// Construct a Coder that writes to a given Output and that will write
    /// `len` bytes.
    ExampleCoder(Env& env, Output& out, size_t len) {
    }

    /// Destructor, should finalize any work on the Output stream.
    ~ExampleCoder() {
    }

    /// Encodes a raw symbol (ie, byte) of the input.
    void encode_sym(uint8_t sym) {
    }

    /// Encodes a factor of the supported type.
    void encode_fact(const F& fact) {
    }

    /// Decodes and defactorizes the input
    static void decode(Input& in, Output& out) {
    }
};

}

#endif
