#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/Coder.hpp>

namespace tdc {

/// \brief Defines data encoding to and decoding from a stream of binary
///        integer representations.
///
/// All values are encoded a binary, using as many bits as necessary to store
/// the maximum value of the respective \ref Range.
class BitCoder : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("coder", "bit", "Encoding using range-optimal bit widths");
        return m;
    }

    BitCoder() = delete;

    /// \brief Encodes data to a binary stream.
    class Encoder : public tdc::Encoder {
    public:
        using tdc::Encoder::Encoder;
    };

    /// \brief Decodes data from a binary stream.
    class Decoder : public tdc::Decoder {
    public:
        using tdc::Decoder::Decoder;
    };
};

}

