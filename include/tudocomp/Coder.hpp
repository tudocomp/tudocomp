#ifndef _INCLUDED_CODER_HPP_
#define _INCLUDED_CODER_HPP_

#include <tudocomp/io.hpp>
#include <tudocomp/Algorithm.hpp>

namespace tdc {

/*abstract*/
class Encoder : public Algorithm {

private:
    std::unique_ptr<io::OutputStream> m_outs;

protected:
    std::unique_ptr<BitOStream> m_out;

public:
    inline Encoder(Env&& env, Output& out) : Algorithm(std::move(env)) {
        m_outs = std::make_unique<io::OutputStream>(out.as_stream());
        m_out = std::make_unique<BitOStream>(*m_outs);
    }

    inline ~Encoder() {
        finalize();
    }

    inline void finalize() {
        m_out->flush();
    }
};

/*abstract*/
class Decoder : public Algorithm {

private:
    std::unique_ptr<io::InputStream> m_ins;

protected:
    std::unique_ptr<BitIStream> m_in;

public:
    inline Decoder(Env&& env, Input& in) : Algorithm(std::move(env)) {
        m_ins = std::make_unique<io::InputStream>(in.as_stream());
        m_in = std::make_unique<BitIStream>(*m_ins);
    }

    inline bool eof() const {
        return m_in->eof();
    }
};

}

#endif
