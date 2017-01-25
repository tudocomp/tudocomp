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
        m.option("string_coder").templated<string_coder_t>();
        return m;
    }

    template<typename ref_coder_t>
    class Compression: public Algorithm {
        Env m_ref_env;
        std::shared_ptr<BitOStream> m_out;

        std::vector<size_t> m_refs;
        std::vector<size_t> m_ref_ranges;
        std::vector<uliteral_t> m_strings;

    public:
        inline Compression(Env&& env,
                           Env&& ref_env,
                           std::shared_ptr<BitOStream> out):
            Algorithm(std::move(env)),
            m_ref_env(std::move(ref_env)),
            m_out(out) {}

        inline void encode(Factor fact, size_t ref_range) {
            m_refs.push_back(fact.ref);
            m_ref_ranges.push_back(ref_range);
            //std::cout << "encode: " << vec_to_debug_string(fact.string) << "\n";
            for (auto c : fact.string) {
                m_strings.push_back(c);
            }
            m_strings.push_back(0);
        }

        inline void encode_ref(size_t ref, size_t ref_range) {

        }

        inline void encode_str(View str) {

        }

        inline void encode_sep(bool val) {

        }

        inline void encode_char(uliteral_t c) {

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
                ViewLiterals(m_strings)
            };

            size_t strings_i = 0;

            for (size_t i = 0; i < m_refs.size(); i++) {
                ref_coder.encode(m_refs[i], Range(m_ref_ranges[i]));

                m_out->write_bit(!OVER_THRESHOLD_FLAG);

                while (true) {
                    string_coder.encode(m_strings[strings_i], literal_r);
                    if (m_strings[strings_i] == 0) {
                        strings_i++;
                        break;
                    }
                    strings_i++;
                }
            }
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

        inline size_t decode_ref(size_t ref_range) {
            return 0;
        }

        inline uliteral_t decode_char() {
            return 0;
        }

        inline View decode_str() {
            return "";
        }

        inline bool decode_sep() {
            return false;
        }

        inline Factor decode(size_t ref_range) {
            auto ref = m_ref_coder.template decode<size_t>(Range(ref_range));

            bool is_over_threshold = false;
            is_over_threshold = m_in->read_bit();
            DCHECK(is_over_threshold);

            m_buf.clear();
            while (true) {
                auto c = m_string_coder.template decode<uliteral_t>(literal_r);
                if (c == 0) {
                    break;
                }
                m_buf.push_back(c);
            }

            return Factor {
                m_buf,
                ref
            };
        }

        inline bool eof() {
            return m_ref_coder.eof();
        }

        inline ~Decompression() {

        }
    };
};


}
}
