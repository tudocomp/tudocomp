#ifndef _INCLUDED_COMPRESSOR_HPP_
#define _INCLUDED_COMPRESSOR_HPP_

#include <tudocomp/Env.hpp>
#include <tudocomp/io.h>
#include <tudocomp/Algorithm.hpp>
#include <functional>
#include <memory>

namespace tudocomp {

/// Interface for a general compressor.
class Compressor: public Algorithm {
public:
    /// Construct the class with an environment.
    inline Compressor(Env&& env): Algorithm(std::move(env)) {}

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

using CompressorConstructor = std::function<std::unique_ptr<Compressor>(Env&&)>;


}

#endif
