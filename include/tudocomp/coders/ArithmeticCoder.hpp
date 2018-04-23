#pragma once

#include <sstream>
#include <tudocomp/Coder.hpp>
#include <iostream>

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

    // TODO: Figure out how to correctly replace this with len_t or len_compact_t
    using len_fixup_t = uint32_t;

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
        std::vector<len_fixup_t> C;

        ulong lower_bound=0;
        ulong upper_bound=std::numeric_limits<ulong>::max();
        uliteral_t codebook_size=0;

        len_t literal_count = 0;
        len_t literal_counter = 0;
        ulong min_range=std::numeric_limits<len_fixup_t>::max();

        /**
         * @brief count_alphabet_literals counts how often a literal occurs in \ref input
         * @param input
         * @return an array with count of each single literal
         */
        template<class T>
        std::vector<len_fixup_t> count_alphabet_literals(T&& input) {
            std::vector<len_fixup_t> C;
            C.resize(ULITERAL_MAX+1, 0);

            while(input.has_next()) {
                uliteral_t c = input.next().c;
                DCHECK_LT(static_cast<uliteral_t>(c), ULITERAL_MAX+1);
                DCHECK_LT(C[static_cast<uliteral_t>(c)], std::numeric_limits<len_fixup_t>::max());
                ++C[static_cast<uliteral_t>(c)];
            }

            return C;
        }

        /**
         * @brief build_intervals transforms the counts to an interval-mapping.
         * Every entry contains the difference to the entry before.
         * @param c
         */
        void build_intervals(std::vector<len_fixup_t> &c) {
            if(c[0] != 0u) {
                codebook_size++;
            }
            len_t min=std::numeric_limits<len_fixup_t>::max();
            //calculates difference to the entry before, searches min and counts entries != 0
            for(ulong i=1; i<=ULITERAL_MAX; i++) {
                if(c[i]!=0u) {
                    codebook_size++;
                    min=std::min(min, len_t(c[i]));
                }
                c[i] = c[i] + c[i-1];
            }
            literal_count = c[ULITERAL_MAX-1];

            //normalize all Intervals
            for(ulong i=0; i<=ULITERAL_MAX; i++) {
                c[i] = c[i] / min;
            }
            min_range=c[ULITERAL_MAX-1];
        }


        template<typename value_t>
        inline void setNewBounds(value_t v) {
            ulong range = upper_bound-lower_bound;
            //check if all characters can be uniquely mapped to range
            if(range<min_range){
                writeCode(lower_bound);
                lower_bound=0;
                upper_bound=std::numeric_limits<ulong>::max();
                range = upper_bound-lower_bound;
            }
            DCHECK_NE(lower_bound,upper_bound);

            const ulong literal_count = C[ULITERAL_MAX];

            //unsure if this is needed
            const ulong offset_upper = range <= literal_count ? (range*C[(int) v])/literal_count : (range/literal_count)*C[(int) v];
            upper_bound=lower_bound+offset_upper;
            if(v != 0) { //first interval starts at zero
                const ulong offset_lower = range <= literal_count ? (range*C[(int) v-1])/literal_count : (range/literal_count)*C[(int) v-1];
                lower_bound=lower_bound+offset_lower;
            }

        }

        inline void writeCodebook() {
            ///the written file has following format (without linebreaks):
            /// #all literals in text
            /// #count of entries in codebook
            /// 'character' 'value'
            /// 'character2' 'value2'
            /// code1 code2 code3 code4

            //write count of expected chars
            m_out->write_int<len_fixup_t>(literal_count);

            //write codebook size in outstream
            m_out->write_int<uliteral_t>(codebook_size);

            if(C[0]!=0u) {
                m_out->write_int(uliteral_t(0));
                m_out->write_int(C[0]);
            }
            for(ulong i=1; i<=ULITERAL_MAX;i++) {
                if(C[i]!=C[i-1]) {
                    m_out->write_int((uliteral_t) i);
                    m_out->write_int(C[i]);
                }

            }
        }

        inline void writeCode(ulong code){
            m_out->write_int(code);
        }

        //write last code-block
        void postProcessing() {
            writeCode(lower_bound);
            //dummy codeblock - needed to read until no more code-blocks
            m_out->write_int(std::numeric_limits<ulong>::max());
        }

    public:
        template<typename literals_t>
        inline Encoder(Env&& env, Output& out, literals_t&& literals)
            : Encoder(std::move(env), std::make_shared<BitOStream>(out), literals) {
        }

        template<typename literals_t>
        inline Encoder(Env&& env, std::shared_ptr<BitOStream> out, literals_t&& literals)
            : tdc::Encoder(std::move(env), out, literals),
        C(count_alphabet_literals(std::move(literals))) {
            build_intervals(C);
            writeCodebook();
        }

        using tdc::Encoder::encode; // default encoding as fallback

        template<typename value_t>
        inline void encode(value_t v, const LiteralRange&) {
            literal_counter++;
            setNewBounds(v);

            if(literal_counter==literal_count){
                postProcessing();
            }
        }

        inline void flush() {
            throw std::runtime_error("ArithmeticCoder::flush not implemented");
        }
    };

    /// \brief Decodes data from an Arithmetic character stream.
    class Decoder : public tdc::Decoder {
    private:
        std::vector<std::pair<uliteral_t ,int>> literals;
        std::string decoded;
        uliteral_t codebook_size;
        len_t literal_count = 0;
        len_t literal_counter = 0;
        ulong literals_read = 0;
        ulong min_range=std::numeric_limits<ulong>::max();

        void decode(ulong code) {
            ulong lower_bound = 0;
            ulong upper_bound = std::numeric_limits<ulong>::max();
            std::ostringstream os;
            ulong interval_parts = literals[codebook_size -1].second;
            //count of characters in stream

            ulong range = upper_bound - lower_bound;

            //stop decoding code-bllock when range is too small or all literals read
            while(min_range<=range && literal_counter<literal_count) {
                ulong interval_lower_bound = lower_bound;
                //search the right interval
                for(int i = 0; i < codebook_size ; i++) {
                    const std::pair<uliteral_t, int>& pair=literals[i];
                    const ulong offset = range <= interval_parts ? range*pair.second/interval_parts : range/interval_parts*pair.second;
                    upper_bound = lower_bound + offset;
                    if(code < upper_bound) {
                        //character decoded
                        os << pair.first;
                        lower_bound = interval_lower_bound;
                        break;
                    }
                    interval_lower_bound = upper_bound;
                }
                literal_counter++;
                range = upper_bound - lower_bound;
            }

            decoded = os.str();
            literals_read = 0;
        }

    public:
        DECODER_CTOR(env, in) {
            //read codebook size
            literal_count = m_in->read_int<len_fixup_t>();
            codebook_size = m_in->read_int<uliteral_t>();
            literals.resize(codebook_size);

            //read and parse dictionary - is is already "normalized"
            for (int i =0; i<codebook_size; i++) {
                uliteral_t c = m_in->read_int<uliteral_t>();
                int val = m_in->read_int<int>();
                literals[i]=std::pair<uliteral_t, int>(c, val);
            }

            min_range=literals[codebook_size-1].second;
        }

        using tdc::Decoder::decode;

        template<typename value_t>
        inline value_t decode(const LiteralRange&) {
            //read code if nothing buffered
            if(!decoded.size()) {
                ulong code = m_in->read_int<ulong>();
                if(code!=std::numeric_limits<ulong>::max()) {
                    //code must not be a dummy-code
                    decode(code);
                }
            }
            value_t val = decoded[literals_read++];

            //if all buffered literals are read: decode the next buffer
            if(literals_read == decoded.size()) {
                ulong code = m_in->read_int<ulong>();
                if(code!=std::numeric_limits<ulong>::max()) {
                    //code must not be a dummy-code
                    decode(code);
                }
            }

            return val;
        }
    };
};

}
