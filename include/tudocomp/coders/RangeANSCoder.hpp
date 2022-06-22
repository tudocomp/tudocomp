#pragma once

#include <array>
#include <bits/ranges_algo.h>
#include <bits/ranges_base.h>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <ios>
#include <iostream>
#include <iterator>
#include <limits>
#include <math.h>
#include <ranges>
#include <sstream>
#include <stdint.h>
#include <string>
#include <sys/types.h>
#include <tudocomp/Coder.hpp>
#include <unistd.h>
#include <vector>

namespace tdc {

/**
 * @brief A Range Asymmetric Numeral System with a 64-bit state and 32-bit normalization.
 *
 * This method was first decribed by Jaroslaw (Jarek) Duda in his paper from 2013.
 * https://arxiv.org/abs/1311.2540
 *
 * See also his video describing the method: https://www.youtube.com/watch?v=R5PeBBQw190
 *
 * Note that the text is decoded in reverse order.
 * It is also vitally important, that the Encoder is deconstructed before starting the decoding process with the
 * decoder. (That is, before constructing the Decoder.)
 *
 * @tparam range_size_exp The exponent of the size of the range that the natural numbers will be partitioned in. So the
 * size of the range will be 2^range_size_exp. This is "n" in Duda's video.
 */
template<size_t range_size_exp>
class RangeANSCoder : public Algorithm {

    static_assert(range_size_exp > 0, "Cannot be encoded with width 0");
    static_assert(range_size_exp <= sizeof(size_t) * 8 - 1, "Encoding cannot be greater than the word width");

    /**
     * @RANGE_SIZE The size of the ranges that the natural numbers are partitioned in. The larger the range, the more
     * accurate the probability function Note that if range_size_exp is too small for the text, then this can result in
     * the histogram containing 0s. In that case this might lead to division-by zero errors
     */
    static const size_t RANGE_SIZE = 1 << range_size_exp;

    /**
     * @MASK The mask used to replace an (x % RANGE_SIZE) operation with (x & MASK) since RANGE_SIZE is a power of 2.
     */
    static const size_t MASK = RANGE_SIZE - 1;

    /**
     * @SIZE_T_BITS The number of bits in a size_t
     */
    static const size_t SIZE_T_BITS = sizeof(size_t) * 8;

    /**
     * @TARGET The starting and end state of the de/encoding process
     */
    static const uint64_t TARGET = 1ull << 32;

  public:
    inline static Meta meta() {
        Meta m(Coder::type_desc(), "rans", "Range Asymmetric Numeral System coder");
          m.param("range_size_exp", "The size of the range, partitioning the natural numbers. The size will be 2^range_size_exp").primitive(10);
        return m;
    }
    /// \cond DELETED
    RangeANSCoder() = delete;
    /// \endcond

    /// \brief Encodes data from an ASCII character stream.
    class Encoder : public tdc::Encoder {

        /**
         * @m_state The current state of the encoding process. This is "x" in Duda's video.
         * Whenever this number grows too large during encoding, the lower 32bit are written to the output.
         * When this object is deconstructed the resulting state in its entirety is also written to the output
         */
        uint64_t m_state;

        /**
         * @m_hist A histogram of the ASCII characters occuring in the input. This is normalized so the total sum of all
         * values is equal to RANGE_SIZE. This is "f" in Duda's video.
         */
        std::array<size_t, 256> m_hist;

        /**
         * @m_cdf A (not-really) cumulative distribution function over the histogram. The value at index i is the sum
         * of all values in [0..i) in m_hist This is "c" in Duda's video
         */
        std::array<size_t, 256> m_cdf;

        /**
         * @m_input_size The size of the input text.
         */
        size_t m_input_size;

      public:
        using tdc::Encoder::Encoder;

        inline static Meta meta() {
            Meta m(Coder::type_desc(), "rans_enc", "Range ANS encoding");
            return m;
        }

      private:
        /**
         * @brief Creates an unnormalized histogram of the characters in the input text.
         *
         * @tparam Literals The type of the input text. This should be a sized_range
         * @param literals The input text
         */
        template<std::ranges::sized_range Literals>
        void histogram(Literals &literals) {
            m_hist.fill(0);
            for (auto &literal : literals) {
                m_hist[literal]++;
            }
        }

        /**
         * @brief This normalizes the histogram such that the sum of all values in m_hist sum up to RANGE_SIZE.
         * In addition, this populates m_cdf
         */
        void normalize_hist() {
            srand(0);
            static std::array<double, 256> prob_cdf;
            prob_cdf[0] = 0.0;
            m_cdf[0]    = 0;

            for (int i = 1; i < 256; i++) {
                prob_cdf[i]   = prob_cdf[i - 1] + (double) m_hist[i - 1] / (double) m_input_size;
                size_t unrounded_value = prob_cdf[i] * RANGE_SIZE;
                m_cdf[i]      = unrounded_value < 1 ? ceil(unrounded_value) : unrounded_value;
                m_hist[i - 1] = m_cdf[i] - m_cdf[i - 1];
            }
            m_hist[255] = RANGE_SIZE - m_cdf[255];
        }

        /**
         * @brief This writes the input size, m_hist and m_cdf to the output
         */
        void write_aux_ds() {
            m_out->write_int(m_input_size, SIZE_T_BITS);
            for (int i = 0; i < 256; i++) {
                m_out->write_int(m_hist[i], SIZE_T_BITS);
            }
            for (int i = 0; i < 256; i++) {
                m_out->write_int(m_cdf[i], SIZE_T_BITS);
            }
        }

      public:
        /**
         * @brief Encodes a value.
         *
         * @tparam Value The input value type. This must be a type corresponding to an 8-bit number.
         * @param s The value to encode.
         * @param r Unused. Using different ranges will mess up the encoding.
         */
        template<typename Value>
        inline void encode(Value s, const Range &r) {
            // Renormalize by writing out the lower 32 bits.
            if (m_state >= (m_hist[s] << (64 - range_size_exp))) {
                uint32_t write = 0xFFFFFFFF & m_state;
                m_state >>= 32;
                m_out->write_int(write, 32);
            }
            m_state = ((m_state / m_hist[s]) << range_size_exp) + (m_state % m_hist[s]) + m_cdf[s];
        }

        /**
         * @brief Returns the length of the input in bytes.
         *
         * @return The number of bytes in the input.
         */
        inline size_t input_size() const { return m_input_size; }

        template<typename literals_t>
        inline Encoder(Config &&cfg, Output &out, literals_t &&literals) :
            Encoder(std::move(cfg), std::make_shared<BitOStream>(out), literals) {}

        template<std::ranges::sized_range literals_t>
        inline Encoder(Config &&cfg, std::shared_ptr<BitOStream> out, literals_t &&literals) :
            tdc::Encoder(std::move(cfg), out, literals),
            m_state{1ull << 32},
            m_input_size{std::ranges::size(literals)} {

            // Compute the histogram
            histogram(literals);
            // Normalize it
            normalize_hist();
            // Write m_hist and m_cdf to the output.
            write_aux_ds();
        }

        /**
         * @brief Clean up by writing the state to the output.
         * We need to write the state at the very end.
         */
        ~Encoder() {
            // When we're done, we write the final state to the Output
            m_out->write_int(m_state, 64);
        }
    };

    /// \brief Decodes data from an ASCII character stream.
    class Decoder : public tdc::Decoder {

        /**
         * @m_state The current state of the decoding process. This is "x" in Duda's video.
         * Whenever this number grows too large during encoding, the lower 32bit are written to the output.
         * When this object is deconstructed the resulting state in its entirety is also written to the output
         */
        uint64_t m_state;

        /**
         * @m_hist A histogram of the ASCII characters occuring in the input. This is normalized so the total sum of all
         * values is equal to RANGE_SIZE
         */
        std::array<size_t, 256> m_hist;

        /**
         * @m_cdf A (not-really) cumulative distribution function over the histogram. The value at index i is the sum 
         * of all values in [0..i) in m_hist
         */
        std::array<size_t, 256> m_cdf;

        /**
         * @m_buf The buffer holding the entire encoded number split into 32bit numbers for convenience.
         */
        std::vector<uint32_t> m_buf;

        /**
         * @m_input_size The size of the input text.
         */
        size_t m_input_size;

        /**
         * @brief Reads the input size, m_hist and m_cdf from the input.
         */
        void read_aux_ds() {
            m_input_size = m_in->read_int<size_t>(SIZE_T_BITS);
            for (int i = 0; i < 256; i++) {
                m_hist[i] = m_in->read_int<size_t>(SIZE_T_BITS);
            }
            for (int i = 0; i < 256; i++) {
                m_cdf[i] = m_in->read_int<size_t>(SIZE_T_BITS);
            }
        }

        /**
         * @brief Reads m_buf from the input.
         */
        void read_data() {
            while (!m_in->eof()) {
                m_buf.push_back(m_in->read_int<uint32_t>(32));
            }
            // The last two written u32 must be the final state
            uint64_t lo = m_buf.back();
            m_buf.pop_back();
            uint64_t hi = m_buf.back();
            m_buf.pop_back();
            m_state = (hi << 32) | lo;
        }

        /**
         * @brief Calculates the next symbol that is to be decoded from the current state.
         *
         * @return The char that is the next symbol in the state.
         */
        char symbol() {
            // Find first element such that symbol is greater or equal than it
            // max { s: value <= element } <=>
            // min { s: value > element } - 1
            auto val = std::ranges::upper_bound(m_cdf.cbegin(), m_cdf.cend(), m_state & MASK);

            // Find its index
            auto i = std::distance(m_cdf.cbegin(), val) - 1;
            return i;
        }

      public:
        inline static Meta meta() {
            Meta m(Coder::type_desc(), "rans_dec", "Range ANS decoding");
            return m;
        }

        using tdc::Decoder::Decoder;

        /**
         * @brief Decodes a value.
         *
         * @tparam Value The type of the return value. This will always return an 8-bit wide number.
         * @param r Unused. Using different ranges will mess up the encoding.
         * @return The next character decoded from the state 
         */
        template<typename Value>
        inline Value decode(const Range &r) {
            char s  = symbol();
            m_state = m_hist[s] * (m_state >> range_size_exp) + (m_state & MASK) - m_cdf[s];
            // Renormalize by reading the next 32 bits
            if (m_state < (1ull << 32)) {
                m_state <<= 32;
                m_state += m_buf.back();
                m_buf.pop_back();
            }
            return s;
        }

        /**
         * @brief Checks whether the encoding is done.
         *
         * @return true, if the encoding is done, false otherwise.
         */
        inline bool is_done() const { return m_state == TARGET; }

        /**
         * @brief Returns the length of the input in bytes.
         *
         * @return The number of bytes in the input.
         */
        inline size_t input_size() const { return m_input_size; }

        inline Decoder(Config &&cfg, Input &in) : Decoder(std::move(cfg), std::make_shared<BitIStream>(in)) {}

        inline Decoder(Config &&cfg, std::shared_ptr<BitIStream> in) : tdc::Decoder(std::move(cfg), in), m_state{0} {
            read_aux_ds();
            read_data();
        }
    };
};

} // namespace tdc
