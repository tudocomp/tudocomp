/**
 *
 * This file contains code snippets from the documentation as a reference.
 *
 * Please do not change this file unless you change the corresponding snippets
 * in the documentation as well!
 *
**/

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <tudocomp/tudocomp.hpp>

#include "../test/util.hpp"

using namespace tdc;

// "Capsule" for Encoder and Decoder implementations
class MyCoder : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("coder", "my_coder", "An example coder");
        return m;
    }

    MyCoder() = delete;

    class Encoder : public tdc::Encoder {
    private:
        // keep occurences of each character
        int m_occ[256];

    public:
        template<typename literals_t>
        inline Encoder(Env&& env, Output& out, literals_t&& literals)
            : tdc::Encoder(std::move(env), out, literals) {

            // count occurences of each literal
            std::memset(m_occ, 0, 256);

            while(literals.has_next()) {
                Literal l = literals.next();
                ++m_occ[l.c];
            }
        }

        template<typename value_t>
        inline void encode(value_t v, const Range& r) {
            // Compute the amount of bits required to store a value of range r
            auto delta_bits = bits_for(r.delta());

            // Encode the value as binary using the computed amount of bits
            v -= r.min();
            m_out->write_int(v, delta_bits);
        }

        template<typename value_t>
        inline void encode(value_t v, const BitRange& r) {
            // Encode single bits as ASCII
            m_out->write_int(v ? '1' : '0');
        }

        inline int occ(uliteral_t c) {
            return m_occ[c];
        }
    };

    class Decoder : public tdc::Decoder {
    public:
        using tdc::Decoder::Decoder;

        template<typename value_t>
        inline value_t decode(const Range& r) {
            // Compute the amount of bits required to store a value of range r
            auto delta_bits = bits_for(r.delta());

            // Decode the value from binary reading the computed amount of bits
            value_t v = m_in->read_int<value_t>(delta_bits);
            v += r.min();
            return v;
        }

        template<typename value_t>
        inline value_t decode(const BitRange& r) {
            // Decode an ASCII character and compare against '0'
            uint8_t b = m_in->read_int<uint8_t>();
            return (b != '0');
        }
    };
};

TEST(doc_coder_impl, test) {
    std::stringstream ss;
    
    Range r1(75, 125);
    Range r2(699, 702);

    {
        Output out(ss);
        MyCoder::Encoder encoder(create_env(MyCoder::meta()), out,
            ViewLiterals("a very simple text"));

        ASSERT_EQ(3, encoder.occ(' '));

        encoder.encode(100, r1);
        encoder.encode(true, bit_r);
        encoder.encode(700, r2);
    }

    std::string result = ss.str();
    {
        Input in(result);
        MyCoder::Decoder decoder(create_env(MyCoder::meta()), in);

        ASSERT_EQ(100, decoder.template decode<int>(r1));
        ASSERT_EQ(true, decoder.template decode<bool>(bit_r));
        ASSERT_EQ(700, decoder.template decode<int>(r2));
        ASSERT_TRUE(decoder.eof());
    }
}

