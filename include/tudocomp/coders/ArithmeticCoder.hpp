#pragma once

#include <limits>
#include <sstream>

#include <tudocomp/Coder.hpp>

namespace tdc {

/// \brief Defines data encoding to and decoding from a stream of ASCII
///        characters.
///
/// Literals are encoded by their character representation, bits are
/// encoded using either the character \e 0 or \e 1 and integers are
/// encoded as their string representations, terminated by the \e :
/// character.
class ArithmeticCoder : public Algorithm {
    using ulong = unsigned long;
    using codebook_index_t = uint16_t;

    static constexpr codebook_index_t STOP = ULITERAL_MAX + 1;
    static constexpr codebook_index_t MAX_LITERAL = STOP;

public:
    /// \brief Yields the coder's meta information.
    /// \sa Meta
    inline static Meta meta() {
        Meta m(Coder::type_desc(), "arithmetic", "Simple range encoding");
        return m;
    }

    /// \cond DELETED
    ArithmeticCoder() = delete;
    /// \endcond

    /// \brief Encodes data to an ASCII character stream.
    class Encoder : public tdc::Encoder {
    private:
        ulong m_lb, m_ub;
        codebook_index_t m_codebook_size;

        std::vector<len_compact_t> m_c;
        len_compact_t m_num_literals;

        inline bool code_dirty() const {
            // test if "current code" is dirty and needs flushing
            return (m_lb > 0 || m_ub < ULONG_MAX);
        }

        template<class literals_t>
        inline void compute_histogram(literals_t&& literals) {
            m_num_literals = 0;
            m_c.resize(MAX_LITERAL+1, 0);

            while(literals.has_next()) {
                uliteral_t c = literals.next().c;
                DCHECK_LE(c, MAX_LITERAL);
                DCHECK_LT(m_c[c], INDEX_MAX);
                ++m_c[c];
                ++m_num_literals;
            }

            CHECK_GT(m_num_literals, 0U) << "input is empty";

            // insert artifical STOP character
            m_c[STOP] = 1;
            ++m_num_literals;
        }

        /**
         * @brief build_intervals transforms the counts to an interval-mapping.
         * Every entry contains the difference to the entry before.
         * @param c
         */
        inline void build_intervals() {
            // compute C array, and find amount of codebook entries
            if(m_c[0] != 0U) {
                ++m_codebook_size;
            }

            for(ulong i = 1; i <= MAX_LITERAL; i++) {
                if(m_c[i] != 0U) {
                    ++m_codebook_size;
                }
                m_c[i] += m_c[i-1];
            }

            DCHECK_EQ(m_c[MAX_LITERAL], m_num_literals);
        }

        inline void write_codebook() {
            m_out->write_int(m_codebook_size);
            if(m_c[0] != 0u) {
                m_out->write_int(codebook_index_t(0));
                m_out->write_int(m_c[0]);
            }

            for(codebook_index_t c = 1; c <= MAX_LITERAL; c++) {
                if(m_c[c] != m_c[c-1]) {
                    m_out->write_int(c);
                    m_out->write_int(m_c[c]);
                }
            }
        }

        inline bool encode_next(codebook_index_t c) {
            // update interval
            const ulong range = m_ub - m_lb;
            DCHECK_GE(range, m_num_literals);

            m_ub = m_lb + (range / m_num_literals) * m_c[c];
            if(c > 0U) { // first interval starts at zero
                m_lb += (range / m_num_literals) * m_c[c - 1];
            }

            DCHECK_NE(m_lb, m_ub);

            if(m_ub - m_lb < m_num_literals) {
                // write code and reset bounds
                m_out->write_int(m_lb);
                m_lb = 0;
                m_ub = ULONG_MAX;
                return true;
            } else {
                // code not yet "complete"
                return false;
            }
        }

    public:
        template<typename literals_t>
        inline Encoder(Config&& cfg, Output& out, literals_t&& literals)
            : Encoder(std::move(cfg), std::make_shared<BitOStream>(out), literals) {
        }

        template<typename literals_t>
        inline Encoder(Config&& cfg, std::shared_ptr<BitOStream> out, literals_t&& literals)
            : tdc::Encoder(std::move(cfg), out, literals),
              m_lb(0),
              m_ub(ULONG_MAX),
              m_codebook_size(0) {

            compute_histogram(literals);
            build_intervals();
            write_codebook();
        }

        inline ~Encoder() {
            flush();
        }

        using tdc::Encoder::encode; // default encoding as fallback

        template<typename value_t>
        inline void encode(value_t v, const LiteralRange&) {
            encode_next(codebook_index_t(v));
        }

        inline void flush() {
            if(code_dirty()) {
                // fill current code with STOP signs
                while(!encode_next(STOP));
            }
        }
    };

    /// \brief Decodes data from an Arithmetic character stream.
    class Decoder : public tdc::Decoder {
    private:
        using codebook_entry_t = std::pair<codebook_index_t, len_compact_t>;

        std::vector<codebook_entry_t> m_codebook;
        len_compact_t m_num_literals;

        std::string m_decode_buffer;
        ulong m_decoded;

        inline void read_codebook() {
            // header
            auto codebook_size = m_in->read_int<codebook_index_t>();

            // initialize intervals (already normalized)
            m_codebook.resize(codebook_size);
            for(codebook_index_t i =0; i < codebook_size; i++) {
                auto c = m_in->read_int<codebook_index_t>();
                auto v = m_in->read_int<len_compact_t>();

                m_codebook[i] = codebook_entry_t(c, v);
            }

            DCHECK_EQ(STOP, m_codebook[codebook_size - 1].first);
            m_num_literals = m_codebook[codebook_size - 1].second;
        }

        inline void decode(ulong code) {
            std::ostringstream buf;

            ulong lb = 0;
            ulong ub = ULONG_MAX;
            ulong range = ub - lb;
            while(range >= m_num_literals) {
                // find the right interval
                ulong next_lb = lb;
                codebook_index_t c = STOP;

                for(codebook_index_t i = 0; i < m_codebook.size() ; i++) {
                    const codebook_entry_t& e = m_codebook[i];
                    ub = lb + (range / m_num_literals) * e.second;

                    if(code < ub) {
                        lb = next_lb;
                        c = e.first;
                        break;
                    }

                    next_lb = ub;
                }

                if(c != STOP) {
                    // decoded a valid character
                    DCHECK_LE(c, ULITERAL_MAX);
                    buf << uliteral_t(c);
                    range = ub - lb;

                } else {
                    // code finished
                    break;
                }
            }

            m_decode_buffer = buf.str();
            m_decoded = 0;
        }

    public:
        inline Decoder(Config&& cfg, Input& in)
             : Decoder(std::move(cfg), std::make_shared<BitIStream>(in)) {
        }

        inline Decoder(Config&& cfg, std::shared_ptr<BitIStream> in)
            : tdc::Decoder(std::move(cfg), in),
              m_decoded(0) {

            read_codebook();
        }

        using tdc::Decoder::decode;

        inline bool eof() const {
            if(m_decoded < m_decode_buffer.size()) {
                // still decoding
                return false;
            } else {
                // check if stream is over
                return m_in->eof();
            }
        }

        template<typename value_t>
        inline value_t decode(const LiteralRange&) {
            if(m_decoded >= m_decode_buffer.size()) {
                // read and decode next code
                auto code = m_in->read_int<ulong>();
                decode(code);
                DCHECK_LT(m_decoded, m_decode_buffer.size());
            }

            return value_t(m_decode_buffer[m_decoded++]);
        }
    };
};

}
