#pragma once
#include "tdc/uint/uint40.hpp"
#include "tudocomp/ds/uint_t.hpp"
#include <array>
#include <bits/ranges_algo.h>
#include <bits/ranges_base.h>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <limits>
#include <math.h>
#include <ranges>
#include <string>
#include <tudocomp/Coder.hpp>
#include <unistd.h>

namespace tdc {

/// \brief Codes each symbol as a binary number of the specified number of bits.
///
template<size_t range_size_exp>
class RangeANSCoder : public Algorithm {

    static_assert(range_size_exp > 0, "Cannot be encoded with width 0");
    static_assert(range_size_exp <= sizeof(size_t) * 8 - 1, "Encoding cannot be greater than the word width");

    static const size_t RANGE_SIZE = 1 << range_size_exp;
    static const size_t MASK       = RANGE_SIZE - 1;
    static const size_t SIZE_T_BITS = sizeof(size_t) * 8;

  public:
    inline static Meta meta() {
        Meta m(Coder::type_desc(), "rans", "Range Asymmetric Numeral System coder");
        return m;
    }
    /// \cond DELETED
    RangeANSCoder() = delete;
    /// \endcond

    //TODO Find some way to encode the final state and to decode it again
    //TODO Implement decoding step 
    /// \brief Encodes data to an ASCII character stream.
    class Encoder : public tdc::Encoder {
        uint64_t                m_buf;
        std::array<size_t, 256> m_hist;
        std::array<size_t, 256> m_cdf;
        size_t                  m_input_size;

      public:
        using tdc::Encoder::Encoder;

        inline static Meta meta() {
            Meta m(Coder::type_desc(), "arithmetic", "Simple range encoding");
            return m;
        }

        template<std::ranges::sized_range Literals>
        void histogram(Literals &literals) {
            m_hist.fill(0);
            for (auto &literal : literals) {
                m_hist[literal]++;
            }
        }

        void normalize_hist() {
            srand(0);
            std::array<double, 256> prob_cdf;
            prob_cdf[0] = 0.0;
            m_cdf[0]    = 0;

            for (int i = 1; i < 256; i++) {
                prob_cdf[i]   = prob_cdf[i - 1] + (double) m_hist[i - 1] / (double) m_input_size;
                m_cdf[i]      = prob_cdf[i] * RANGE_SIZE;
                m_hist[i - 1] = m_cdf[i] - m_cdf[i - 1];
            }
            m_hist[255] = RANGE_SIZE - m_cdf[255];
        }

        void write_aux_ds() {
            for (int i = 0; i < 256; i++) {
                m_out->write_int(m_hist[i], SIZE_T_BITS);
            }
            for (int i = 0; i < 256; i++) {
                m_out->write_int(m_cdf[i], SIZE_T_BITS);
            }
        }

        uint8_t symbol(size_t state) {
            // Find first element such that symbol is greater or equal than it
            auto val = std::ranges::upper_bound(m_cdf.cbegin(), m_cdf.cend(), state - 1);

            // Find its index
            auto i = std::distance(m_cdf.cbegin(), val);
            return i;
        }

        void code(uint8_t s) {
            if (m_buf >= (m_hist[s] << (64 - range_size_exp))) {
                uint32_t write = 0xFFFFFFFF & m_buf;
                m_buf >>= 32;
                m_out->write_int(write, 32);
            }
            m_buf = (((uint64_t) floor(m_buf / (double) m_hist[s])) << range_size_exp) + (m_buf % m_hist[s]) + m_cdf[s];
        }

        template<typename value_t>
        inline void encode(value_t v, const Range &r) {
            uint8_t s = symbol(m_buf & MASK);
            code(s);
        }

        template<typename literals_t>
        inline Encoder(Config &&cfg, Output &out, literals_t &&literals) :
            Encoder(std::move(cfg), std::make_shared<BitOStream>(out), literals) {}

        template<std::ranges::sized_range literals_t>
        inline Encoder(Config &&cfg, std::shared_ptr<BitOStream> out, literals_t &&literals) :
            tdc::Encoder(std::move(cfg), out, literals),
            m_input_size{std::ranges::size(literals)},
            m_buf{1ull << 32}{

            histogram(literals);
            normalize_hist();
            write_aux_ds();      
        }
    };

    /// \brief Decodes data from an ASCII character stream.
    class Decoder : public tdc::Decoder {
        
        static const uint64_t TARGET = 1ull << 32;
        std::array<size_t, 256> m_hist;
        std::array<size_t, 256> m_cdf;

        void read_aux_ds() {
            for (int i = 0; i < 256; i++) {
                m_hist[i] = m_in->read_int<size_t>(SIZE_T_BITS);
            }
            for (int i = 0; i < 256; i++) {
                m_cdf[i] = m_in->read_int<size_t>(m_cdf[i]);
            }
        }

      public:
        using tdc::Decoder::Decoder;

        template<typename value_t>
        inline value_t decode(const Range &r) {
            return r.min() + m_in->read_int<value_t>(range_size_exp);
        }

        inline Decoder(Config&& cfg, Input& in)
             : Decoder(std::move(cfg), std::make_shared<BitIStream>(in)) {}

        inline Decoder(Config&& cfg, std::shared_ptr<BitIStream> in)
            : tdc::Decoder(std::move(cfg), in) {
            read_aux_ds();           
        }
    };
};

} // namespace tdc
