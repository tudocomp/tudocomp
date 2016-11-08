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

}

#endif
