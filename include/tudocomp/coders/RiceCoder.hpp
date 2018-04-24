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
        Meta m("coder", "rice", "Rice coding");
        m.option("p").dynamic(5); // b = 2^p
        return m;
    }

    RiceCoder() = delete;

    /// \brief Encodes data to a stream of Elias-Delta codes.
    class Encoder : public tdc::Encoder {
    private:
        uint8_t m_p;

    public:
        template<typename literals_t>
        inline Encoder(Env&& env, std::shared_ptr<BitOStream> out, literals_t&& literals)
            : tdc::Encoder(std::move(env), out, literals),
              m_p(this->env().option("p").as_integer()) {
        }

        template<typename literals_t>
        inline Encoder(Env&& env, Output& out, literals_t&& literals)
            : Encoder(std::move(env), std::make_shared<BitOStream>(out), literals) {
        }

        using tdc::Encoder::encode;

        template<typename value_t>
        inline void encode(value_t v, const Range&) {
            m_out->write_rice(v, m_p);
        }
    };

    /// \brief Decodes data from a stream of Elias-Delta codes.
    class Decoder : public tdc::Decoder {
    private:
        uint8_t m_p;

    public:
        inline Decoder(Env&& env, std::shared_ptr<BitIStream> in)
            : tdc::Decoder(std::move(env), in),
              m_p(this->env().option("p").as_integer()) {
        }

        inline Decoder(Env&& env, Input& in)
            : Decoder(std::move(env), std::make_shared<BitIStream>(in)) {
        }

        using tdc::Decoder::decode;

        template<typename value_t>
        inline value_t decode(const Range&) {
            return m_in->read_rice<value_t>(m_p);
        }
    };
};

}

