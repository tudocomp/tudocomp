#ifndef _INCLUDED_CODER_HPP_
#define _INCLUDED_CODER_HPP_

#include <tudocomp/io.hpp>
#include <tudocomp/Algorithm.hpp>

namespace tdc {

class Coder : public Algorithm {

private:
    std::unique_ptr<io::OutputStream> m_outs;
    std::unique_ptr<io::InputStream> m_ins;
    bool m_ins_done;

protected:
    std::unique_ptr<BitOStream> m_out;
    std::unique_ptr<BitIStream> m_in;

public:
    inline Coder() = delete;
    inline Coder(Env&& env) : Algorithm(std::move(env)) {}

    inline void encode_init(Output& out) {
        m_outs = std::make_unique<io::OutputStream>(out.as_stream());
        m_out = std::make_unique<BitOStream>(*m_outs);
    }

    inline void decode_init(Input& in) {
        m_ins = std::make_unique<io::InputStream>(in.as_stream());
        m_in = std::make_unique<BitIStream>(*m_ins, m_ins_done);
    }
};

}

#endif
