#pragma once

#include <tudocomp/io.hpp>
#include <tudocomp/Algorithm.hpp>

namespace tdc {

/// \brief Base for decompressors.
class Decompressor : public Algorithm {
public:
    static inline constexpr TypeDesc type_desc() {
        return TypeDesc("decompressor");
    }

    virtual ~Decompressor() = default;
    Decompressor(Decompressor const&) = default;
    Decompressor(Decompressor&&) = default;
    Decompressor& operator=(Decompressor const&) = default;
    Decompressor& operator=(Decompressor&&) = default;

    using Algorithm::Algorithm;

    /// \brief Decompress the given input to the given output.
    ///
    /// \param input The input.
    /// \param output The output.
    virtual void decompress(Input& input, Output& output) = 0;
};

}

