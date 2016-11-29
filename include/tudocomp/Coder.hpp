#ifndef _INCLUDED_CODER_HPP_
#define _INCLUDED_CODER_HPP_

#include <tudocomp/io.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Literal.hpp>
#include <tudocomp/Range.hpp>

namespace tdc {

/*abstract*/
class Encoder : public Algorithm {

protected:
    std::shared_ptr<BitOStream> m_out;

public:
    inline Encoder(Env&& env, std::shared_ptr<BitOStream> out)
        : Algorithm(std::move(env)), m_out(out) {
    }

    inline ~Encoder() {
        finalize();
    }

    template<typename value_t>
    inline void encode(value_t v, const Range& r) {
        m_out->write_int(v - r.min(), bits_for(r.max() - r.min()));
    }

    template<typename value_t>
    inline void encode(value_t v, const BitRange&) {
        m_out->write_bit(v);
    }

    template<typename value_t>
    inline void encode_unary(value_t v) {
        while(v--) {
            m_out->write_bit(0);
        }
        m_out->write_bit(1);
    }

    template<typename value_t>
    inline void encode_elias_gamma(value_t v) {
        encode_unary(bits_for(v));
        m_out->write_int(v, bits_for(v));
    }

    template<typename value_t>
    inline void encode_elias_delta(value_t v) {
        encode_elias_gamma(bits_for(v));
        m_out->write_int(v, bits_for(v));
    }

    inline void finalize() {
    }
};

#define ENCODER_CTOR(env, out, literals)                                   \
        template<typename literals_t>                                      \
        inline Encoder(Env&& env, Output& out, literals_t&& literals) \
             : Encoder(std::move(env),                                     \
                      std::make_shared<BitOStream>(out), std::move(literals)) {}      \
        template<typename literals_t>                                      \
        inline Encoder(Env&& env, std::shared_ptr<BitOStream> out, literals_t&& literals) \
            : tdc::Encoder(std::move(env), out)

/*abstract*/
class Decoder : public Algorithm {

protected:
    std::shared_ptr<BitIStream> m_in;

public:
    inline Decoder(Env&& env, std::shared_ptr<BitIStream> in)
        : Algorithm(std::move(env)), m_in(in) {
    }

    inline bool eof() const {
        return m_in->eof();
    }

    template<typename value_t>
    inline value_t decode(const Range& r) {
        return value_t(r.min()) +
               m_in->read_int<value_t>(bits_for(r.max() - r.min()));
    }

    template<typename value_t>
    inline value_t decode(const BitRange&) {
        return value_t(m_in->read_bit());
    }

    template<typename value_t>
    inline value_t decode_unary() {
        value_t v = 0;
        while(!m_in->read_bit()) ++v;
        return v;
    }

    template<typename value_t>
    inline value_t decode_elias_gamma() {
        auto bits = decode_unary<size_t>();
        return m_in->read_int<value_t>(bits);
    }

    template<typename value_t>
    inline value_t decode_elias_delta() {
        auto bits = decode_elias_gamma<size_t>();
        return m_in->read_int<value_t>(bits);
    }
};

#define DECODER_CTOR(env, in)                                \
        inline Decoder(Env&& env, Input& in)                 \
             : Decoder(std::move(env),                       \
                       std::make_shared<BitIStream>(in)) {}  \
        inline Decoder(Env&& env, std::shared_ptr<BitIStream> in) \
            : tdc::Decoder(std::move(env), in)

}

#endif
