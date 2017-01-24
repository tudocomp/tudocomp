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

        inline void encode(Factor fact, size_t ref_range) {
            m_ref_coder.encode(fact.ref, Range(ref_range));

            bool is_over_threshold = true;
            m_out->write_bit(is_over_threshold);

            for (auto c : fact.string) {
                m_string_coder.encode(c, literal_r);
            }
            m_string_coder.encode(0, literal_r);
        }

        class Callback {
            Compression* m_comp;
            friend class Compression;
        public:
            void encode_below_threshold(View str) {

            }

            void encode_above_threshold() {

            }

            void encode_above_threshold_char() {

            }

            void encode_above_threshold_ref() {

            }
        };

        template<typename F>
        inline void encode_nested(size_t ref, size_t ref_range, F f) {
            m_ref_coder.encode(ref, Range(ref_range));
            f(Callback(this));
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
    };
};


}
}
