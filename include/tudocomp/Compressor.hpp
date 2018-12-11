#pragma once

#include <functional>
#include <memory>

#include <tudocomp/io.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Decompressor.hpp>

namespace tdc {

/// \brief Base for data compressors.
class Compressor: public Algorithm {
public:
    static inline constexpr TypeDesc type_desc() {
        return TypeDesc("compressor");
    }

    virtual ~Compressor() = default;
    Compressor(Compressor const&) = default;
    Compressor(Compressor&&) = default;
    Compressor& operator=(Compressor const&) = default;
    Compressor& operator=(Compressor&&) = default;

    using Algorithm::Algorithm;

    /// \brief Compress the given input to the given output.
    ///
    /// \param input The input.
    /// \param output The output.
    virtual void compress(Input& input, Output& output) = 0;

    /// \brief Returns a decompressor.
    virtual std::unique_ptr<Decompressor> decompressor() const = 0;
};

}

