#pragma once

#include <tudocomp/Coder.hpp>

namespace tdc {

/// \brief Defines data encoding to and decoding from a stream of
///        Elias-Gamma codes.
///
/// All values are encoded using Elias-Gamma code.
class EliasGammaCoder : public Algorithm {
public:
    inline static Meta meta() {
        Meta m(Coder::type_desc(), "gamma",
            "Encodes integers using Elias-gamma codes.");
        return m;
    }

    EliasGammaCoder() = delete;

    /// \brief Encodes data to a stream of Elias-Gamma codes.
    class Encoder : public tdc::Encoder {
    public:
        using tdc::Encoder::Encoder;
        using tdc::Encoder::encode;

        template<typename value_t>
        inline void encode(value_t v, const Range&) {
            m_out->write_elias_gamma(v);
        }
    };

    /// \brief Decodes data from a stream of Elias-Gamma codes.
    class Decoder : public tdc::Decoder {
    public:
        using tdc::Decoder::Decoder;
        using tdc::Decoder::decode;

        template<typename value_t>
        inline value_t decode(const Range&) {
            return m_in->read_elias_gamma<value_t>();
        }
    };
};

}

