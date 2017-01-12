#pragma once

#include <tudocomp/Algorithm.hpp>

namespace tdc {
namespace lz78u {

class AsciiNt: public Algorithm {
public:
    using Algorithm::Algorithm;

    inline static Meta meta() {
        Meta m("lz78u_string_coder", "ascii_nt",
               "Codes string as a sequence of Ascii bytes terminated with a 0 byte.");
        return m;
    }

    class Encoder: public Algorithm {
        std::shared_ptr<BitOStream> m_out;
    public:
        inline Encoder(Env&& env, const std::shared_ptr<BitOStream>& out):
            Algorithm(std::move(env)), m_out(out) {}

        inline void encode(View str) {
            for (auto c : str) {
                m_out->write_int<uliteral_t>(c);
            }
            m_out->write_int<uliteral_t>(0);
        }
    };

    class Decoder: public Algorithm {
        std::shared_ptr<BitIStream> m_in;
    public:
        inline Decoder(Env&& env, const std::shared_ptr<BitIStream>& in):
            Algorithm(std::move(env)), m_in(in) {}

        inline void decode(std::vector<uliteral_t>& buffer) {
            while (true) {
                const uliteral_t chr = m_in->read_int<uliteral_t>();
                if (chr == '\0') {
                    break;
                }
                buffer.push_back(chr);
            }
        }
    };
};

}
}//ns
