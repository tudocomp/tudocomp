#pragma once

#include <tudocomp/Coder.hpp>

namespace tdc {

/// \brief Defines data encoding to and decoding from a stream of
///        Elias-Delta codes.
///
/// All values are encoded using Elias-Delta code.
class EliasDeltaCoder : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("coder", "delta", "Elias-delta encoding");
        return m;
    }

    EliasDeltaCoder() = delete;

    /// \brief Encodes data to a stream of Elias-Delta codes.
    class Encoder : public tdc::Encoder {
    public:
        using tdc::Encoder::Encoder;
        using tdc::Encoder::encode;

        template<typename value_t>
        inline void encode(value_t v, const Range& r) {
            m_out->write_elias_delta(v - value_t(r.min()));
        }
    };

    /// \brief Decodes data from a stream of Elias-Delta codes.
    class Decoder : public tdc::Decoder {
    public:
        using tdc::Decoder::Decoder;
        using tdc::Decoder::decode;

        template<typename value_t>
        inline value_t decode(const Range& r) {
            return value_t(r.min()) + m_in->read_elias_delta<value_t>();
        }
    };
};

}

