#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/Coder.hpp>

#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/ds/Rank.hpp>

namespace tdc {

class SigmaCoder : public Algorithm {
public:
    inline static Meta meta() {
        Meta m(Coder::type_desc(), "sigma",
            "Encode characters using log(sigma) bits.");
        return m;
    }

    SigmaCoder() = delete;

    class Encoder : public tdc::Encoder {
    private:
        BitVector m_bv;
        Rank m_rank;
        size_t m_sigma_bits;

    public:
        template<typename literals_t>
        inline Encoder(Config&& cfg, std::shared_ptr<BitOStream> out, literals_t&& literals)
            : tdc::Encoder(std::move(cfg), out, literals) {

            // find occuring characters
            m_bv = BitVector(ULITERAL_MAX+1, 0);
            while(literals.has_next()) {
                m_bv[literals.next().c] = 1;
            }

            // employ rank support
            m_rank = Rank(m_bv);

            // determine alphabet size
            const size_t sigma = m_rank(ULITERAL_MAX);

            // store alphabet
            const size_t literal_bits = bits_for(ULITERAL_MAX);
            m_out->write_int(sigma, literal_bits);
            for(size_t c = 0; c <= ULITERAL_MAX; c++) {
                if(m_bv[c]) m_out->write_int(c, literal_bits);
            }

            // bits
            m_sigma_bits = bits_for(sigma-1);
        }

        template<typename literals_t>
        inline Encoder(Config&& cfg, Output& out, literals_t&& literals)
            : Encoder(std::move(cfg), std::make_shared<BitOStream>(out), literals) {
        }

        using tdc::Encoder::encode; // default encoding as fallback

        template<typename value_t>
        inline void encode(value_t v, const LiteralRange&) {
            DCHECK_GE(m_rank(v), 1);
            m_out->write_int(m_rank(v)-1, m_sigma_bits);
        }
    };

    class Decoder : public tdc::Decoder {
    private:
        uliteral_t* m_alphabet;
        size_t m_sigma_bits;

    public:
        inline Decoder(Config&& cfg, std::shared_ptr<BitIStream> in)
            : tdc::Decoder(std::move(cfg), in) {

            // read alphabet
            const size_t literal_bits = bits_for(ULITERAL_MAX);
            const size_t sigma = m_in->read_int<size_t>(literal_bits);

            m_alphabet = new uliteral_t[sigma];
            for(size_t i = 0; i < sigma; i++) {
                m_alphabet[i] = m_in->read_int<uliteral_t>(literal_bits);
            }

            m_sigma_bits = bits_for(sigma-1);
        }

        inline Decoder(Config&& cfg, Input& in)
            : Decoder(std::move(cfg), std::make_shared<BitIStream>(in)) {
        }

        ~Decoder() {
            delete[] m_alphabet;
        }

        using tdc::Decoder::decode; // default decoding as fallback

		template<typename value_t>
		inline value_t decode(const LiteralRange&) {
            const size_t i = m_in->read_int<size_t>(m_sigma_bits);
            return m_alphabet[i];
        }
    };
};

}
