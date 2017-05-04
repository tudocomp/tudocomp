#pragma once

#include "pre_header.hpp"

namespace tdc {
namespace lz78u {

template<typename string_coder_t>
class BufferingStrategy: public Algorithm {
public:
    using Algorithm::Algorithm;

    inline static Meta meta() {
        Meta m("lz78u_strategy", "buffering");
        m.option("string_coder").templated<string_coder_t>("coder");
        return m;
    }

    template<typename ref_coder_t>
    class Compression: public Algorithm {
        Env m_ref_env;
        std::shared_ptr<BitOStream> m_out;

        // TODO Optimization: Replace with something that just stores the increment points
        std::vector<Range> m_ref_ranges;
        // TODO Optimization: Replace with variable bit width
        std::vector<size_t> m_refs;
        // TODO Optimization: Replace with bitvector
        std::vector<bool> m_seps;
        std::vector<uliteral_t> m_chars;

        // TODO Optimization: Replace with 2-bit vector, or remove completely
        // by hardcoding the encoding scheme in the strategy
        // TODO Optimization: Encode "encode_str" case more compactly by using single code?
        std::vector<uint8_t> m_stream;

    public:
        inline Compression(Env&& env,
                           Env&& ref_env,
                           std::shared_ptr<BitOStream> out):
            Algorithm(std::move(env)),
            m_ref_env(std::move(ref_env)),
            m_out(out) {}

        inline void encode_ref(size_t ref, Range ref_range) {
            m_refs.push_back(ref);
            m_ref_ranges.push_back(ref_range);
            m_stream.push_back(0);
        }

        inline void encode_sep(bool val) {
            m_seps.push_back(val);
            m_stream.push_back(1);
        }

        inline void encode_char(uliteral_t c) {
            m_chars.push_back(c);
            m_stream.push_back(2);
        }

        inline void encode_str(View str) {
            for (auto c : str) {
                encode_char(c);
            }
            encode_char(0);
        }

        inline ~Compression() {
            typename ref_coder_t::Encoder ref_coder {
                std::move(m_ref_env),
                m_out,
                NoLiterals()
            };
            typename string_coder_t::Encoder string_coder {
                std::move(this->env().env_for_option("string_coder")),
                m_out,
                ViewLiterals(m_chars)
            };

            auto refs_i = m_refs.begin();
            auto ref_ranges_i = m_ref_ranges.begin();
            auto seps_i = m_seps.begin();
            auto chars_i = m_chars.begin();

            for (auto kind : m_stream) {
                switch (kind) {
                    case 0: {
                        auto ref = *(refs_i++);
                        auto ref_range = *(ref_ranges_i++);
                        ref_coder.encode(ref, ref_range);

                        break;
                    }
                    case 1: {
                        auto sep = *(seps_i++);
                        m_out->write_bit(sep);

                        break;
                    }
                    case 2: {
                        auto chr = *(chars_i++);
                        string_coder.encode(chr, literal_r);

                        break;
                    }
                }
            }

            DCHECK(refs_i == m_refs.end());
            DCHECK(ref_ranges_i == m_ref_ranges.end());
            DCHECK(seps_i == m_seps.end());
            DCHECK(chars_i == m_chars.end());
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
