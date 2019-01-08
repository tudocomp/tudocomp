#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/Coder.hpp>

namespace tdc {

/// \brief Defines data encoding to and decoding from a stream of binary
///        integer representations.
///
/// All values are encoded a binary, using as many bits as necessary to store
/// the maximum value of the respective \ref Range.
class BinaryCoder : public Algorithm {
public:
    inline static Meta meta() {
        Meta m(Coder::type_desc(), "binary",
            "Encodes integers in their binary representation. An integer "
            "from a range [M,N] is encoded using ceil(log(N-M)) bits.");
        return m;
    }

    BinaryCoder() = delete;

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

