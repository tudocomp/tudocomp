#ifndef _INCLUDED_CODER_HPP_
#define _INCLUDED_CODER_HPP_

#include <tudocomp/io.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Literal.hpp>

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

    inline void finalize() {
    }
};

#define ENCODER_CTOR(env, out, literals)                                   \
        template<typename literals_t>                                      \
        inline Encoder(Env&& env, Output& out, const literals_t& literals) \
             : Encoder(std::move(env),                                     \
                      std::make_shared<BitOStream>(out), literals) {}      \
        template<typename literals_t>                                      \
        inline Encoder(Env&& env, std::shared_ptr<BitOStream> out, const literals_t& literals) \
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
};

#define DECODER_CTOR(env, in)                                \
        inline Decoder(Env&& env, Input& in)                 \
             : Decoder(std::move(env),                       \
                       std::make_shared<BitIStream>(in)) {}  \
        inline Decoder(Env&& env, std::shared_ptr<BitIStream> in) \
            : tdc::Decoder(std::move(env), in)

}

#endif
