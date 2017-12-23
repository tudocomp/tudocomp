#pragma once

#include <functional>
#include <memory>

#include <tudocomp/io.hpp>
#include <tudocomp/Algorithm.hpp>

namespace tdc {

/// \brief Base for data compressors.
///
/// A \e compressor defines the two operations \e compress and \e decompress.
/// The \e compress operation transforms an input into a different
/// representation with the goal of reducing the required memory. The
/// \e decompress operation restores the original input from such a
/// representation.
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

    /// \brief Construct the compressor with an environment.
    ///
    /// \param env The algorithm's environment.
    inline Compressor(Env&& env): Algorithm(std::move(env)) {}

    /// \brief Compress the given input to the given output.
    ///
    /// \param input The input.
    /// \param output The output.
    virtual void compress(Input& input, Output& output) = 0;

    /// \brief Decompress the given input to the given output.
    ///
    /// \param input The input.
    /// \param output The output.
    virtual void decompress(Input& input, Output& output) = 0;
};

}

