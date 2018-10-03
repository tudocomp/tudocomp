#pragma once

#include <tudocomp/Coder.hpp>

namespace tdc {

/// \brief Defines data encoding to and decoding from a stream of
///        Elias-Delta codes.
///
/// All values are encoded using Elias-Delta code.
class RiceCoder : public Algorithm {
public:
    inline static Meta meta() {
        Meta m(Coder::type_desc(), "rice",
            "Encodes integers using Rice codes (gamma code of quotient, "
            "truncated binary code of remainder).");
        m.param("p", "The divisor exponent (the divisor is b = 2^p)")
            .primitive(5); // b = 2^p
        return m;
    }

    RiceCoder() = delete;

    /// \brief Encodes data to a stream of Elias-Delta codes.
    class Encoder : public tdc::Encoder {
    private:
        uint8_t m_p;

    public:
        template<typename literals_t>
        inline Encoder(Config&& cfg, std::shared_ptr<BitOStream> out, literals_t&& literals)
            : tdc::Encoder(std::move(cfg), out, literals),
              m_p(this->config().param("p").as_uint()) {
        }

        template<typename literals_t>
        inline Encoder(Config&& cfg, Output& out, literals_t&& literals)
            : Encoder(std::move(cfg), std::make_shared<BitOStream>(out), literals) {
        }

        using tdc::Encoder::encode;

        template<typename value_t>
        inline void encode(value_t v, const Range& r) {
            m_out->write_rice(v - value_t(r.min()), m_p);
        }
    };

    /// \brief Decodes data from a stream of Elias-Delta codes.
    class Decoder : public tdc::Decoder {
    private:
        uint8_t m_p;

    public:
        inline Decoder(Config&& cfg, std::shared_ptr<BitIStream> in)
            : tdc::Decoder(std::move(cfg), in),
              m_p(this->config().param("p").as_uint()) {
        }

        inline Decoder(Config&& cfg, Input& in)
            : Decoder(std::move(cfg), std::make_shared<BitIStream>(in)) {
        }

        using tdc::Decoder::decode;

        template<typename value_t>
        inline value_t decode(const Range& r) {
            return value_t(r.min()) + m_in->read_rice<value_t>(m_p);
        }
    };
};

}

