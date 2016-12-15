#pragma once

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
};

#define DECODER_CTOR(env, in)                                \
        inline Decoder(Env&& env, Input& in)                 \
             : Decoder(std::move(env),                       \
                       std::make_shared<BitIStream>(in)) {}  \
        inline Decoder(Env&& env, std::shared_ptr<BitIStream> in) \
            : tdc::Decoder(std::move(env), in)

}

