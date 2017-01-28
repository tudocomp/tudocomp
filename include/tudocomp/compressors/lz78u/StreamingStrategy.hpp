#pragma once

#include "pre_header.hpp"

namespace tdc {
namespace lz78u {

template<typename string_coder_t>
class StreamingStrategy: public Algorithm {
public:
    using Algorithm::Algorithm;

    inline static Meta meta() {
        Meta m("lz78u_strategy", "streaming");
        m.option("string_coder").templated<string_coder_t>();
        return m;
    }

    template<typename ref_coder_t>
    class Compression: public Algorithm {
        typename ref_coder_t::Encoder     m_ref_coder;
        typename string_coder_t::Encoder  m_string_coder;

        std::shared_ptr<BitOStream> m_out;

    public:
        inline Compression(Env&& env,
                           Env&& ref_env,
                           std::shared_ptr<BitOStream> out):
            Algorithm(std::move(env)),
            m_ref_coder(std::move(ref_env), out, NoLiterals()),
            m_string_coder(std::move(this->env().env_for_option("string_coder")), out, NoLiterals()),
            m_out(out) {}

        inline void encode_ref(size_t ref, Range ref_range) {
            DVLOG(2) << "encode ref: " << ref;
            m_ref_coder.encode(ref, ref_range);
        }

        inline void encode_char(uliteral_t c) {
            DVLOG(2) << "encode char: '" << c << "' (" << int(c) << ")";
            m_string_coder.encode(c, literal_r);
        }

        inline void encode_str(View str) {
            for (auto c : str) {
                encode_char(c);
            }
            encode_char(0);
        }

        inline void encode_sep(bool val) {
            DVLOG(2) << "encode sep: " << int(val == 1);
            m_out->write_bit(val);
        }
    };

    template<typename ref_coder_t>
    class Decompression: public Algorithm {
        typename ref_coder_t::Decoder     m_ref_coder;
        typename string_coder_t::Decoder  m_string_coder;

        std::vector<uliteral_t> m_buf;
        std::shared_ptr<BitIStream> m_in;
    public:
        inline Decompression(Env&& env,
                             Env&& ref_env,
                             std::shared_ptr<BitIStream> in):
            Algorithm(std::move(env)),
            m_ref_coder(std::move(ref_env), in),
            m_string_coder(std::move(this->env().env_for_option("string_coder")), in),
            m_in(in) {}

        inline size_t decode_ref(Range ref_range) {
            return m_ref_coder.template decode<size_t>(ref_range);
        }

        inline uliteral_t decode_char() {
            return m_string_coder.template decode<uliteral_t>(literal_r);
        }

        inline View decode_str() {
            m_buf.clear();
            while (true) {
                auto c = decode_char();
                if (c == 0) {
                    break;
                }
                m_buf.push_back(c);
            }
            return m_buf;
        }

        inline bool decode_sep() {
            return m_in->read_bit();
        }

        inline bool eof() {
            return m_ref_coder.eof();
        }
    };
};


}
}
