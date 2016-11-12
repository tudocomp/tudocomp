#ifndef _INCLUDED_CODER_HPP_
#define _INCLUDED_CODER_HPP_

#include <tudocomp/io.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Literal.hpp>

namespace tdc {

/*abstract*/
class Encoder : public Algorithm {

private:
    io::OutputStream m_outs;

protected:
    std::unique_ptr<BitOStream> m_out;

public:
    inline Encoder(Env&& env, Output& out)
        : Algorithm(std::move(env)), m_outs(out.as_stream()) {

        m_out = std::make_unique<BitOStream>(m_outs);
    }

    inline ~Encoder() {
        finalize();
    }

    inline void finalize() {
    }
};

/*abstract*/
class Decoder : public Algorithm {

private:
    io::InputStream m_ins;

protected:
    std::unique_ptr<BitIStream> m_in;

public:
    inline Decoder(Env&& env, Input& in)
        : Algorithm(std::move(env)), m_ins(in.as_stream()) {

        m_in = std::make_unique<BitIStream>(m_ins);
    }

    inline bool eof() const {
        return m_in->eof();
    }
};

}

#endif
