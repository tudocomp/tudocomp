#pragma once

#include <sstream>
#include <iostream>
#include <bitset>
#include <numeric>

#include <tudocomp/Coder.hpp>

namespace tdc {namespace esp {
    namespace huff2 {
        // TODO: dynamic size in memory, lesser priority
        using OrderedCodelengths = std::vector<size_t>;
        using OrderedMapToEffective = std::vector<size_t>; // TODO < use better map than vector for higher value ranges
        using Codewords = std::vector<size_t>;
        using Counts = std::vector<size_t>;
        using MapFromEffective = std::vector<size_t>;
        using Codelengths = std::vector<size_t>;
        using Numl = std::vector<size_t>;
        using OrderedMapFromEffective = std::vector<size_t>;
        using Firstcode = std::vector<size_t>;

        inline static MapFromEffective gen_effective_alphabet(const Counts& counts, size_t alphabet_size) {
            MapFromEffective map_from_effective;
            map_from_effective.reserve(alphabet_size);
            map_from_effective.resize(alphabet_size);

            size_t j = 0;
            for(size_t i = 0; i < counts.size(); ++i) {
                if(counts[i] == 0) continue;
                DCHECK_LT(j, alphabet_size);
                map_from_effective[j++] = i;
            }
            DCHECK_EQ(j, alphabet_size);
            for(size_t i = 0; i < alphabet_size; ++i) {
                DCHECK_NE(counts[map_from_effective[i]], 0);
            }
            return map_from_effective;
        }

        inline static Codelengths gen_codelengths(Counts counts,
                                           const MapFromEffective& map_from_effective,
                                           size_t alphabet_size) {
            std::vector<size_t> A;
            A.reserve(alphabet_size * 2);
            A.resize(alphabet_size * 2);

            for(size_t i = 0; i < alphabet_size; i++) {
                DVLOG(2)
                    << "Char " << map_from_effective[i]
                    << " : " << size_t(counts[map_from_effective[i]])
                    << "\t";
                A[alphabet_size + i] = counts[map_from_effective[i]];
                A[i] = alphabet_size + i;
            }

            // build a minHeap of A[0..size)
            auto comp = [&A] (const size_t a, const size_t b) -> bool {
                return A[a] > A[b];
            };
            std::make_heap(&A[0], &A[alphabet_size], comp);
            DCHECK_LE(A[A[0]], A[A[1]]);

            DVLOG(2) << "A: " << vec_to_debug_string(A) << "\t";

            size_t h = alphabet_size - 1;
            //builds the Huffman tree

            while(h > 0) {
                std::pop_heap(A.begin(), A.begin() + h + 1, comp);
                const size_t m1 = A[h];  // first element from the heap (already popped)
                --h;

                std::pop_heap(A.begin(), A.begin() + h + 1, comp);
                const size_t m2 = A[h]; //second min count
                DCHECK_GT(m1, h);
                DCHECK_GT(m2, h);

                A[h + 1] = A[m1] + A[m2]; // create a new parent node
                A[h] = h + 1;
                A[m1] = A[m2] = h + 1; //parent pointer
                std::push_heap(A.begin(), A.begin() + h + 1, comp);
            }

            A[1] = 0;
            for(size_t i = 2; i < 2 * alphabet_size; ++i) {
                A[i] = A[A[i]]+ 1;
            }

            Codelengths codelengths;
            codelengths.reserve(alphabet_size);
            codelengths.resize(alphabet_size);
            for (size_t i = 0; i < alphabet_size; i++) {
                DCHECK_LE(A[alphabet_size + i], 64); // the latter representation allows only codewords of length at most 64 bits
                codelengths[i] = A[alphabet_size+i];
                DVLOG(2)
                    << "Char " << map_from_effective[i]
                    << " : " << codelengths[i]
                    << "\t";
            }

            return codelengths;

        }

        inline static Numl gen_numl(const OrderedCodelengths& ordered_codelengths, const size_t alphabet_size, const uint8_t longest) {
            DCHECK_EQ(longest, *std::max_element(ordered_codelengths.begin(),
                                                 ordered_codelengths.end()));
            DCHECK_GT(longest, 0);

            // numl : length l -> #codewords of length l
            Numl numl;
            numl.reserve(longest);
            numl.resize(longest);

            for (size_t i = 0; i < alphabet_size; ++i) {
                DCHECK_LE(ordered_codelengths[i], longest);
                DCHECK_GT(ordered_codelengths[i], 0);
                ++numl[ordered_codelengths[i] - 1];
            }

            return numl;
        }

        inline static Firstcode gen_first_codes(const Numl& numl, const size_t longest) {
            Firstcode firstcode;
            firstcode.reserve(longest);
            firstcode.resize(longest);

            firstcode[longest - 1] = 0;
            for(size_t i = longest-1; i > 0; --i) {
                firstcode[i - 1] = (firstcode[i] + numl[i]) / 2;
            }
            return firstcode;
        }

        inline static Codewords gen_codewords(const OrderedCodelengths& ordered_codelengths,
                                const size_t alphabet_size,
                                const Numl& numl,
                                const uint8_t longest) {
            DCHECK_EQ(longest, *std::max_element(ordered_codelengths.begin(),
                                                 ordered_codelengths.end()));
            DCHECK_GT(longest,0);

            auto firstcode = gen_first_codes(numl, longest);

            Codewords codewords;
            codewords.reserve(alphabet_size);
            codewords.resize(alphabet_size);
            for(size_t i = 0; i < alphabet_size; ++i) {
                DCHECK_LE(ordered_codelengths[i], longest);
                DCHECK_GT(ordered_codelengths[i], 0);
                codewords[i] = firstcode[ordered_codelengths[i]-1]++;
                DVLOG(2) << "codeword " << i << " : " << std::bitset<64>(codewords[i])
                    << ", length " << ordered_codelengths[i]
                    << "\t";
            }
            return codewords;
        }

        inline static OrderedMapToEffective gen_ordered_map_to_effective(
            const OrderedMapFromEffective& ordered_map_from_effective,
            const size_t alphabet_size,
            const size_t real_alphabet_size)
        {

            size_t max = real_alphabet_size;
            // TODO Debug
            //max = std::max(real_alphabet_size, ULITERAL_MAX);
            OrderedMapToEffective map_to_effective;
            map_to_effective.reserve(max);
            map_to_effective.resize(max, size_t(-1));

            for(size_t i = 0; i < alphabet_size; ++i) {
                map_to_effective[ordered_map_from_effective[i]] = i;
            }

            DVLOG(2) << "ordered_map_from_effective : "
                << vec_to_debug_string(ordered_map_from_effective)
                << "\t";
            DVLOG(2) << "map_to_effective : "
                << vec_to_debug_string(map_to_effective)
                << "\t";

            return map_to_effective;
        }


        struct Huffmantable {
            OrderedMapFromEffective m_ordered_map_from_effective;
            size_t m_effective_alphabet_size;
            Numl m_numl;
            size_t m_longest;
            size_t m_real_alphabet_size;
        };

        struct ExtendedHuffmantable : Huffmantable {
            Codewords m_codewords;
            OrderedCodelengths m_ordered_codelengths;

            inline void gen_huffmantable(Counts&& counts, size_t alphabet_size) {
                DCHECK_GT(alphabet_size, 0);

                OrderedCodelengths ordered_codelengths;
                OrderedMapFromEffective ordered_map_from_effective;
                size_t longest;

                {
                    auto map_from_effective = gen_effective_alphabet(counts, alphabet_size);
                    auto codelengths = gen_codelengths(std::move(counts),
                                                    map_from_effective,
                                                    alphabet_size);
                    std::vector<size_t> codeword_order;
                    codeword_order.reserve(alphabet_size);
                    codeword_order.resize(alphabet_size);
                    std::iota(codeword_order.begin(), codeword_order.end(), 0);
                    std::sort(codeword_order.begin(), codeword_order.end(),
                              [&] (size_t i, size_t j) -> bool {
                                  return codelengths[i] < codelengths[j];
                              });
                    longest = *std::max_element(codelengths.begin(),
                                                codelengths.end());

                    ordered_codelengths.reserve(alphabet_size);
                    ordered_codelengths.resize(alphabet_size);

                    ordered_map_from_effective.reserve(alphabet_size);
                    ordered_map_from_effective.resize(alphabet_size);

                    for(size_t i = 0; i < alphabet_size; ++i) {
                        ordered_codelengths[i] = codelengths[codeword_order[i]];
                        ordered_map_from_effective[i] = map_from_effective[codeword_order[i]];
                    }
                }

                auto numl = gen_numl(ordered_codelengths, alphabet_size, longest);
                auto codewords = gen_codewords(ordered_codelengths, alphabet_size, numl, longest);

                m_ordered_map_from_effective = std::move(ordered_map_from_effective);
                m_codewords = std::move(codewords);
                m_ordered_codelengths = std::move(ordered_codelengths);
                m_effective_alphabet_size = std::move(alphabet_size);
                m_numl = std::move(numl);
                m_longest = std::move(longest);
            }

            template<typename input_t>
            ExtendedHuffmantable(const input_t& inp) {
                size_t max = 0;
                for (size_t i = 0; i < inp.size(); i++) {
                    max = std::max(max, inp[i]);
                }
                size_t alphabet_size = max + 1;

                Counts counts;
                counts.reserve(alphabet_size);
                counts.resize(alphabet_size);
                for (size_t i = 0; i < inp.size(); i++) {
                    counts[inp[i]]++;
                }

                m_real_alphabet_size = alphabet_size;

                size_t shrunk_alphabet_size = 0;
                for (size_t i = 0; i < counts.size(); i++) {
                    shrunk_alphabet_size += (counts[i] != 0);
                }

                if (shrunk_alphabet_size <= 1) {
                    m_effective_alphabet_size = shrunk_alphabet_size;
                } else {
                    gen_huffmantable(std::move(counts), shrunk_alphabet_size);
                }
            }
        };

        inline static void huffman_encode(
            size_t input,
            tdc::io::BitOStream& os,
            const OrderedCodelengths& ordered_codelengths,
            const OrderedMapToEffective& ordered_map_to_effective,
            const size_t alphabet_size,
            const Codewords& codewords)
        {
            const size_t char_value = input;
            const size_t effective_char = ordered_map_to_effective[char_value];
            DCHECK_LT(effective_char, alphabet_size);
            os.write_int(codewords[effective_char], ordered_codelengths[effective_char]);
            DVLOG(2) << " codeword "
                << codewords[effective_char]
                << " length " << int(ordered_codelengths[effective_char])
                << "\t";
        }

        // TODO: Only write one huffman table for both D arrays
        // -> just treat both d arrays as one array
        inline static void huffmantable_encode(BitOStream& os, const Huffmantable& table) {
            os.write_compressed_int(table.m_real_alphabet_size);
            os.write_compressed_int(table.m_longest);
            for(size_t i = 0; i < table.m_longest; ++i) {
                os.write_compressed_int(table.m_numl[i]);
            }
            os.write_compressed_int(table.m_effective_alphabet_size);
            for(size_t i = 0; i < table.m_effective_alphabet_size; ++i) {
                // TODO: variable bit width here
                os.write_int<size_t>(table.m_ordered_map_from_effective[i], bits_for(table.m_real_alphabet_size - 1));
            }
        }


        using PrefixSumLengths = std::vector<size_t>;

        inline static Huffmantable huffmantable_decode(tdc::io::BitIStream& in) {
            const size_t real_size = in.read_compressed_int<size_t>();
            const size_t longest = in.read_compressed_int<size_t>();

            Numl numl;
            numl.reserve(longest);
            numl.resize(longest);

            for(size_t i = 0; i < longest; ++i) {
                numl[i] = in.read_compressed_int<size_t>();
            }

            const size_t alphabet_size = in.read_compressed_int<size_t>();
            OrderedMapFromEffective ordered_map_from_effective;
            ordered_map_from_effective.reserve(alphabet_size);
            ordered_map_from_effective.resize(alphabet_size);
            for(size_t i = 0; i < alphabet_size; ++i) {
                ordered_map_from_effective[i] = in.read_int<size_t>(bits_for(real_size - 1));
            }

            return {
                std::move(ordered_map_from_effective),
                alphabet_size,
                std::move(numl),
                longest,
                real_size,
            };
        }

        inline static OrderedCodelengths gen_ordered_codelengths(
            const size_t alphabet_size, const Numl& numl, const size_t longest)
        {
            OrderedCodelengths ordered_codelengths;
            ordered_codelengths.reserve(alphabet_size);
            ordered_codelengths.resize(alphabet_size);
            for(size_t i = 0, k = 0; i < longest; ++i) {
                for(size_t j = 0; j < numl[i]; ++j) {
                    DCHECK_LT(k, alphabet_size);
                    ordered_codelengths[k++] = i + 1;
                }
            }
            return ordered_codelengths;
        }

        inline static PrefixSumLengths gen_prefix_sum_lengths(
            OrderedCodelengths ordered_codelengths,
            size_t alphabet_size,
            size_t longest)
        {
            PrefixSumLengths prefix_sum_lengths;
            prefix_sum_lengths.reserve(longest);
            prefix_sum_lengths.resize(longest);

            /*
            std::fill(prefix_sum_lengths.begin(),
                      prefix_sum_lengths.end(),
                      std::numeric_limits<size_t>::max());*/

            prefix_sum_lengths[ordered_codelengths[0] - 1] = 0;
            for(size_t i = 1; i < alphabet_size; ++i) {
                if(ordered_codelengths[i - 1] < ordered_codelengths[i]) {
                    prefix_sum_lengths[ordered_codelengths[i] - 1] = i;
                }
            }
            DVLOG(2) << "ordered_codelengths : "
                << vec_to_debug_string(ordered_codelengths)
                << "\t";
            DVLOG(2) << "prefix_sum_lengths : "
                << vec_to_debug_string(prefix_sum_lengths)
                << "\t";
            return prefix_sum_lengths;
        }

        inline static size_t huffman_decode(
            tdc::io::BitIStream& is,
            const OrderedMapFromEffective& ordered_map_from_effective,
            const PrefixSumLengths& prefix_sum_lengths,
            const Firstcode& firstcodes)
        {
            DCHECK(!is.eof());
            size_t value = 0;
            size_t length = 0;
            do {
                DCHECK(!is.eof());
                value = (value << 1) + is.read_bit();
                ++length;
            } while(value < firstcodes[length - 1]);

            DVLOG(2) << " codeword " << value << " length " << length << "\t";
            --length;
    //      DCHECK_LT(prefix_sum_lengths[length]+ (value - firstcodes[length]), alphabet_size);
            return ordered_map_from_effective[prefix_sum_lengths[length]+ (value - firstcodes[length]) ];
        }
    }

    using namespace huff2;

    /// \brief Encodes data to an ASCII character stream.
    class HuffmanEncoder {
        std::shared_ptr<BitOStream> m_out;
        ExtendedHuffmantable m_table;
        OrderedMapToEffective m_ordered_map_to_effective;
    public:
        template<typename input_t>
        HuffmanEncoder(const std::shared_ptr<BitOStream>& out,
                       const input_t& literals):
            m_out(out),
            m_table(literals),
            m_ordered_map_to_effective {
                m_table.m_codewords.empty()
                    ? OrderedMapToEffective()
                    : gen_ordered_map_to_effective(m_table.m_ordered_map_from_effective,
                                                   m_table.m_effective_alphabet_size,
                                                   m_table.m_real_alphabet_size)
            }
        {
            if (m_table.m_effective_alphabet_size <= 1) {
                m_out->write_bit(0);
            } else {
                m_out->write_bit(1);
                huffmantable_encode(*m_out, m_table);
            }
        }

        inline void encode(size_t v) {
            if (m_table.m_effective_alphabet_size == 1) {
                m_out->write_int<size_t>(v);
            } else {
                huffman_encode(v, *m_out, m_table.m_ordered_codelengths, m_ordered_map_to_effective, m_table.m_effective_alphabet_size, m_table.m_codewords);
            }
        }
    };

    /// \brief Decodes data from an Huffman character stream.
    class HuffmanDecoder {
        std::shared_ptr<BitIStream> m_in;
        OrderedMapFromEffective m_ordered_map_from_effective;
        PrefixSumLengths m_prefix_sum_lengths;
        Firstcode m_firstcodes;

    public:
        HuffmanDecoder(const std::shared_ptr<BitIStream>& in):
            m_in(in)
        {
            if(!m_in->read_bit()) {
                return;
            }
            Huffmantable table = huffmantable_decode(*m_in);
            m_ordered_map_from_effective = std::move(table.m_ordered_map_from_effective);
            OrderedCodelengths ordered_codelengths = gen_ordered_codelengths(
                table.m_effective_alphabet_size,
                table.m_numl,
                table.m_longest);
            m_prefix_sum_lengths = gen_prefix_sum_lengths(
                std::move(ordered_codelengths),
                table.m_effective_alphabet_size,
                table.m_longest);
            m_firstcodes = gen_first_codes(table.m_numl, table.m_longest);
        }

        inline size_t decode() {
            if(m_ordered_map_from_effective.empty()) {
                return m_in->read_int<size_t>();
            }
            return huffman_decode(*m_in,
                                  m_ordered_map_from_effective,
                                  m_prefix_sum_lengths,
                                  m_firstcodes);
        }
    };
}}
