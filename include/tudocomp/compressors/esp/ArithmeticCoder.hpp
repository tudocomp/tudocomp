#pragma once

#include <sstream>
#include <tudocomp/Coder.hpp>
#include <iostream>

namespace tdc {namespace esp {

    /// \brief Encodes data to an ASCII character stream.
    class ArithmeticEncoder {
    private:
        using CountMap = std::vector<size_t>;

        CountMap m_count_map;
        size_t m_codebook_size = 0;
        size_t m_literal_count = 0;
        size_t m_min_range = std::numeric_limits<size_t>::max();
        size_t m_literal_counter = 0;

        size_t m_lower_bound = 0;
        size_t m_upper_bound = std::numeric_limits<size_t>::max();

        std::shared_ptr<BitOStream> m_out;

        /**
         * @brief count_alphabet_literals counts how often a literal occurs in \ref input
         * @param input
         * @return a CountMap count of each seen literal
         */
        template<class T>
        CountMap count_alphabet_literals(const T& input) {
            size_t max_value = 0;
            for (size_t i = 0; i < input.size(); i++) {
                max_value = std::max(max_value, input[i]);
            }
            CountMap ret;
            size_t overrite = 0; //ULITERAL_MAX+1
            ret.reserve(std::max(max_value + 1, overrite));
            ret.resize(std::max(max_value + 1, overrite));

            for (size_t i = 0; i < input.size(); i++) {
                size_t val = input[i];
                DCHECK_LT(ret[val], std::numeric_limits<size_t>::max());
                ret[val]++;
            }

            std::cout << vec_to_debug_string(ret) << "\n";

            return ret;
        }

        /**
         * @brief build_intervals transforms the counts to an interval-mapping.
         * Every entry contains the difference to the entry before.
         * @param c
         */
        void build_intervals(CountMap& c) {
            if(c[0]) {
                m_codebook_size++;
            }
            size_t min = std::numeric_limits<size_t>::max();
            //calculates difference to the entry before, searches min and counts entries != 0
            for(size_t i = 1; i < c.size(); i++) {
                if(c[i] != 0) {
                    m_codebook_size++;
                    min = std::min(min, c[i]);
                }
                c[i] = c[i] + c[i - 1]; // ?
            }
            m_literal_count = c[c.size() - 2]; // ???

            //normalize all Intervals
            for(size_t i = 1; i < c.size(); i++) {
                c[i] = c[i] / min;
            }
            m_min_range = c[c.size() - 2]; // ???
        }


        inline void setNewBounds(size_t v) {
            size_t range = m_upper_bound - m_lower_bound;
            //check if all characters can be uniquely mapped to range
            if(range < m_min_range) {
                writeCode(m_lower_bound);
                m_lower_bound = 0;
                m_upper_bound = std::numeric_limits<size_t>::max();
                range = m_upper_bound - m_lower_bound;
            }
            DCHECK_NE(m_lower_bound, m_upper_bound);

            const size_t literal_count = m_count_map[m_count_map.size() - 1];

            //unsure if this is needed
            const size_t offset_upper = range <= literal_count
                ? (range * m_count_map[v]) / literal_count
                : (range / literal_count) * m_count_map[v];

            m_upper_bound = m_lower_bound + offset_upper;

            if(v != 0) { //first interval starts at zero
                const size_t offset_lower = range <= literal_count
                    ? (range * m_count_map[v - 1]) / literal_count
                    : (range / literal_count) * m_count_map[v - 1];
                m_lower_bound = m_lower_bound + offset_lower;
            }
        }

        inline void writeCodebook() {
            ///the written file has following format (without linebreaks):
            /// #all literals in text
            /// #count of entries in codebook
            /// 'character' 'value'
            /// 'character2' 'value2'
            /// code1 code2 code3 code4

            // TODO: Bit compressed integers benutzen

            //write count of expected chars
            m_out->write_int<size_t>(m_literal_count);
            std::cout<< "  "  << __LINE__ << " write " << size_t(m_literal_count) << " \n";

            //write codebook size in outstream
            m_out->write_int<size_t>(m_codebook_size);
            std::cout<< "  "  << __LINE__ << " write " << size_t(m_codebook_size) << " \n";

            if(m_count_map[0] != 0) {
                m_out->write_int<size_t>(0);
                std::cout<< "  "  << __LINE__ << " write " << size_t(0) << " \n";
                m_out->write_int<size_t>(m_count_map[0]);
                std::cout<< "  "  << __LINE__ << " write " << size_t(m_count_map[0]) << " \n";
            }
            for(size_t i = 1; i < m_count_map.size(); i++) {
                if(m_count_map[i] != m_count_map[i-1]) {
                    m_out->write_int<size_t>(i);
                    std::cout<< "    "  << __LINE__ << " write " << size_t(i) << " \n";
                    m_out->write_int<size_t>(m_count_map[i]);
                    std::cout<< "    "  << __LINE__ << " write " << size_t(m_count_map[i]) << " \n";
                }
            }
        }

        inline void writeCode(size_t code) {
            m_out->write_int<size_t>(code);
            std::cout<< "  "  << __LINE__ << " write " << size_t(code) << " \n";
        }

        //write last code-block
        void postProcessing() {
            writeCode(m_lower_bound);
            //dummy codeblock - needed to read until no more code-blocks
            // TODO: Make sure this works with bit optimal encoding
            m_out->write_int<size_t>(std::numeric_limits<size_t>::max());
            std::cout<< "  "  << __LINE__ << " write " << size_t(std::numeric_limits<size_t>::max()) << " \n";
        }

    public:
        template<typename input_t>
        ArithmeticEncoder(const std::shared_ptr<BitOStream>& out,
                          const input_t& literals):
            m_out(out),
            m_count_map(count_alphabet_literals(literals))
        {
            build_intervals(m_count_map);
            writeCodebook();
        }

        inline void encode(size_t v) {
            m_literal_counter++;
            setNewBounds(v);

            if(m_literal_counter == m_literal_count) {
                postProcessing();
            }
        }
    };

    /// \brief Decodes data from an Arithmetic character stream.
    class ArithmeticDecoder {
    private:
        std::shared_ptr<BitIStream> m_in;

        size_t m_codebook_size = 0;
        size_t m_literal_count = 0;
        std::vector<std::pair<size_t, size_t>> m_literals;
        size_t m_min_range = std::numeric_limits<size_t>::max();
        size_t m_literals_read = 0;
        size_t m_literal_counter = 0;

        std::vector<size_t> m_decoded;

        void decode(size_t code) {
            size_t lower_bound = 0;
            size_t upper_bound = std::numeric_limits<size_t>::max();
            std::vector<size_t> os;
            size_t interval_parts = m_literals[m_codebook_size - 1].second;
            //count of characters in stream

            size_t range = upper_bound - lower_bound;

            //stop decoding code-bllock when range is too small or all literals read
            while(m_min_range <= range && m_literal_counter < m_literal_count) {
                size_t interval_lower_bound = lower_bound;

                //search the right interval
                for(size_t i = 0; i < m_codebook_size ; i++) {
                    const std::pair<size_t, size_t>& pair = m_literals[i];

                    const size_t offset = range <= interval_parts
                        ? range * pair.second / interval_parts
                        : range / interval_parts * pair.second;

                    upper_bound = lower_bound + offset;

                    std::cout << "i: " << i << ", code: " << code << ", upper_bound: " << upper_bound << "\n";

                    if(code < upper_bound) {
                        //character decoded
                        os.push_back(pair.first);
                        lower_bound = interval_lower_bound;
                        break;
                    }
                    interval_lower_bound = upper_bound;
                }
                m_literal_counter++;
                range = upper_bound - lower_bound;
            }

            m_decoded = os;

            std::cout << "decoded: " << vec_to_debug_string(m_decoded) << "\n";

            m_literals_read = 0;
        }

    public:
        ArithmeticDecoder(const std::shared_ptr<BitIStream>& in):
            m_in(in)
        {
            //read codebook size
            m_literal_count = m_in->read_int<size_t>();
            std::cout<< "  "  << __LINE__ << " read " << size_t(m_literal_count) << " \n";
            m_codebook_size = m_in->read_int<size_t>();
            std::cout<< "  "  << __LINE__ << " read " << size_t(m_codebook_size) << " \n";

            m_literals.reserve(m_codebook_size);
            m_literals.resize(m_codebook_size);

            //read and parse dictionary - it is already "normalized"
            for (size_t i = 0; i < m_codebook_size; i++) {
                size_t c = m_in->read_int<size_t>();
                std::cout<< "    "  << __LINE__ << " read " << size_t(c) << " \n";
                size_t val = m_in->read_int<size_t>();
                std::cout<< "    "  << __LINE__ << " read " << size_t(val) << " \n";
                m_literals[i] = std::pair<size_t, size_t>(c, val);
            }

            m_min_range = m_literals[m_codebook_size - 1].second;
        }

        inline size_t decode() {
            //read code if nothing buffered
            if(!m_decoded.size()) {
                std::cout<< "  "  << __LINE__ << " cond 1 " << " \n";
                size_t code = m_in->read_int<size_t>();
                std::cout<< "  "  << __LINE__ << " read " << size_t(code) << " \n";
                // TODO: NB! Special care if bit optimal
                if(code != std::numeric_limits<size_t>::max()) {
                    std::cout<< "  "  << __LINE__ << " cond 2 " << " \n";
                    //code must not be a dummy-code
                    decode(code);
                }
            }

            size_t val = m_decoded[m_literals_read++];
            std::cout << "val: " << size_t(val) << ", lread: " << m_literals_read << "\n";

            //if all buffered literals are read: decode the next buffer
            if(m_literals_read == m_decoded.size()) {
                std::cout<< "  "  << __LINE__ << " cond 3 " << " \n";
                size_t code = m_in->read_int<size_t>();
                std::cout<< "  "  << __LINE__ << " read " << size_t(code) << " \n";
                // TODO: NB! Special care if bit optimal
                if(code != std::numeric_limits<size_t>::max()) {
                    std::cout<< "  "  << __LINE__ << " cond 4 " << " \n";
                    //code must not be a dummy-code
                    decode(code);
                }
            }

            return val;
        }
    };
}}
