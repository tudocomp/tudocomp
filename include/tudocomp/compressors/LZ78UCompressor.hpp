#pragma once

#include <tudocomp/Compressor.hpp>
#include <tudocomp/decompressors/WrapDecompressor.hpp>
#include <tudocomp/Error.hpp>
#include <tudocomp/Tags.hpp>

#include <sdsl/cst_fully.hpp>
#include <sdsl/cst_sada.hpp>

#include <tudocomp/Range.hpp>

#include <tudocomp/compressors/lz_common/factorid_t.hpp>

#include "lz78u/SuffixTree.hpp"

#include "lz78u/pre_header.hpp"

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {
namespace lz78u {
    using factorid_t = lz_common::factorid_t;

    // TODO: Define factorid for lz78u uniformly

    class Decompressor {
        std::vector<factorid_t> indices;
        std::vector<uliteral_t> literal_strings;
        std::vector<size_t> start_literal_strings;

        std::vector<uliteral_t> buffer;

        public:
        inline factorid_t ref_at(factorid_t index) const {
            DCHECK_NE(index, 0);
            size_t i = index - 1;
            return indices[i];
        }
        inline View str_at(factorid_t index) const {
            DCHECK_NE(index, 0);
            size_t i = index - 1;
            View ls = literal_strings;

            size_t start = start_literal_strings[i];
            size_t end = 0;
            if ((i + 1) < start_literal_strings.size()) {
                end = start_literal_strings[i + 1];
            } else {
                end = ls.size();
            }

            return ls.slice(start, end);
        }

        inline void decompress(factorid_t index, View literals, std::ostream& out) {
            indices.push_back(index);
            start_literal_strings.push_back(literal_strings.size());
            for (auto c : literals) {
                literal_strings.push_back(c);
            }

            //std::cout << "    indices:         " << vec_to_debug_string(indices) << "\n";
            //std::cout << "    start lit str:   " << vec_to_debug_string(start_literal_strings) << "\n";
            //std::cout << "    literal_strings: " << vec_to_debug_string(literal_strings) << "\n";

            buffer.clear();

            while(true) {
                auto inv_old_size = buffer.size();
                for (size_t i = 0; i < literals.size(); i++) {
                    buffer.push_back(literals[literals.size() - i - 1]);
                }

                DCHECK_EQ(inv_old_size + literals.size(), buffer.size());

                if (index == 0) {
                    break;
                }
                literals = str_at(index);
                index = ref_at(index);
            }

            std::reverse(buffer.begin(), buffer.end());
            //std::cout << "    reconstructed: " << vec_to_debug_string(buffer) << "\n";
            out << buffer;
        }

    };
}

template<typename strategy_t, typename ref_coder_t>
class LZ78UCompressor: public CompressorAndDecompressor {
private:
    using node_type = lz78u::SuffixTree::node_type;

    using RefEncoder = typename ref_coder_t::Encoder;
    using RefDecoder = typename ref_coder_t::Decoder;

    using CompressionStrat
        = typename strategy_t::template Compression<RefEncoder>;
    using DecompressionStrat
        = typename strategy_t::template Decompression<RefDecoder>;

public:
    inline static Meta meta() {
        Meta m(Compressor::type_desc(), "lz78u",
            "Computes the LZ78U factorization of the input.");
        m.param("coder", "The output encoder.")
            .strategy<ref_coder_t>(TypeDesc("coder"));
        m.param("comp", "The factorization strategy.")
            .strategy<strategy_t>(TypeDesc("lz78u_strategy"));
        m.param("threshold", "the minimum factor length").primitive(3);
        m.add_tag(tags::require_sentinel);
        return m;
    }

    using CompressorAndDecompressor::CompressorAndDecompressor;

    virtual void compress(Input& input, Output& out) override {
        StatPhase phase1("lz78u");
        //std::cout << "START COMPRESS\n";
        const len_t threshold = config().param("threshold").as_uint(); //factor threshold
        phase1.log_stat("threshold", threshold);

        auto iview = input.as_view();
        MissingSentinelError::check(iview);

        View T = iview;

        lz78u::SuffixTree::cst_t backing_cst;
        StatPhase::wrap("construct suffix tree", [&]{
            // TODO: Specialize sdsl template for less alloc here
            std::string bad_copy_1 = T.slice(0, T.size() - 1);
            //std::cout << "text: " << vec_to_debug_string(bad_copy_1) << "\n";

            construct_im(backing_cst, bad_copy_1, 1);
        });
        lz78u::SuffixTree ST(backing_cst);

        const size_t max_z = T.size() * bits_for(ST.cst.csa.sigma) / bits_for(T.size());
        phase1.log_stat("max z", max_z);

        sdsl::int_vector<> R(ST.internal_nodes,0,bits_for(max_z));

        len_t pos = 0;
        len_t z = 0;

        typedef lz78u::SuffixTree::node_type node_t;

        CompressionStrat strategy {
            config().sub_config("comp"),
            config().sub_config("coder"),
            std::make_shared<BitOStream>(out)
        };

        len_t factor_count = 0;

        // `begin` and `end` denote a half-open range [begin, end)
        // of characters of the input string
        auto output = [&](len_t begin, len_t end, size_t ref) {
            // if trailing 0, remove
            while(T[end-1] == 0) --end;

            // First, encode the reference
            DVLOG(2) << "reference";
            strategy.encode_ref(ref, Range(factor_count));

            // factorize the factor label if the label is above the threshold
            if (end-begin >= threshold) {
                DVLOG(2) << "factorized";

                // indicate that this is a factorized string
                strategy.encode_sep(false);

                for(len_t pos = begin; pos < end;) {
                    // similar to the normal LZ78U factorization, but does not introduce new factor ids

                    const node_t leaf = ST.select_leaf(ST.cst.csa.isa[pos]);
                    len_t d = 1;
                    node_t parent = ST.root;
                    node_t node = ST.level_anc(leaf, d);
                    while(!ST.cst.is_leaf(node) && R[ST.nid(node)] != 0) {
                        parent = node;
                        node = ST.level_anc(leaf, ++d);
                    } // not a good feature: We lost the factor ids of the leaves, since R only stores the IDs of internal nodes
                    const len_t depth = ST.str_depth(parent);
                    // if the largest factor is not large enough, we only store the current character and move one text position to the right
                    if(depth < threshold) {
                        DVLOG(2) << "sub char";
                        strategy.encode_sep(false);
                        strategy.encode_char(T[pos]);
                        ++pos;
                    } else {
                        DVLOG(2) << "sub ref";
                        strategy.encode_sep(true);
                        strategy.encode_ref(R[ST.nid(parent)], Range(factor_count));
                        pos += depth;
                        // taking the last factor can make the string larger than intended, so we have to store a cut-value at the last position
                        if(pos > end) {
                            // TODO: Is this encoding efficient enough?

                            // 0 factors would not appear here, so use 0 as a escape
                            // char to indicate that a cut-value follows
                            DVLOG(2) << "special sub ref";
                            strategy.encode_sep(true);
                            strategy.encode_ref(0, Range(factor_count));
                            strategy.encode_ref(pos-end, len_r);
                        }
                    }
                }
                // all char sequencens in a factor need to be 0 terminated
                DVLOG(2) << "sub char term";
                strategy.encode_sep(false);
                strategy.encode_char(0);
            } else {
                // else just output the string as a whole

                // indicate that this is not a factorized string
                DVLOG(2) << "plain";
                strategy.encode_sep(true);
                strategy.encode_str(T.slice(begin,end));
            }

            factor_count++;
        };

        DVLOG(2) << "[ compress ]";

        // Skip the trailing 0
        while(pos < T.size() - 1) {
            const node_t l = ST.select_leaf(ST.cst.csa.isa[pos]);
            const len_t leaflabel = pos;

            if(ST.parent(l) == ST.root || R[ST.nid(ST.parent(l))] != 0) {
                const len_t parent_strdepth = ST.str_depth(ST.parent(l));

                //std::cout << "out leaf: [" << (pos+parent_strdepth)  << ","<< (pos + parent_strdepth + 1) << "] ";
                output(pos + parent_strdepth,
                       pos + parent_strdepth + 1,
                       R[ST.nid(ST.parent(l))]);

                pos += parent_strdepth+1;
                ++z;

                DCHECK_EQ(z, factor_count);

                continue;
            }

            len_t d = 1;
            node_t parent = ST.root;
            node_t node = ST.level_anc(l, d);


            while(R[ST.nid(node)] != 0) {
                parent = node;
                node = ST.level_anc(l, ++d);
            }
            pos += ST.str_depth(parent);


            // const auto& str = T.slice(leaflabel + ST.str_depth(parent), leaflabel + ST.str_depth(node));
            const len_t begin = leaflabel + ST.str_depth(parent);
            const len_t end = leaflabel + ST.str_depth(node);

            //std::cout << "out slice: [ "<< (leaflabel + ST.str_depth(parent)) << ", "<< (leaflabel + ST.str_depth(node))<< " ] ";
            output(begin,
                   end,
                   R[ST.nid(ST.parent(node))]);
            R[ST.nid(node)] = ++z;

            pos += end - begin;
            DCHECK_EQ(z, factor_count);
        }
    }

    virtual void decompress(Input& input, Output& output) override final {
        //std::cout << "START DECOMPRESS\n";
        DVLOG(2) << "[ decompress ]";
        auto out = output.as_stream();

        {
            DecompressionStrat strategy {
                config().sub_config("comp"),
                config().sub_config("coder"),
                std::make_shared<BitIStream>(input)
            };

            uint64_t factor_count = 0;

            lz78u::Decompressor decomp;

            std::vector<uliteral_t> rebuilt_buffer;

            while (!strategy.eof()) {
                auto ref = strategy.decode_ref(Range(factor_count));
                DVLOG(2) << "";
                DVLOG(2) << "decode ref: " << ref;
                DVLOG(2) << "";
                bool not_factorized = strategy.decode_sep();
                DVLOG(2) << "decode sep: " << int(not_factorized);

                if (not_factorized) {
                    auto str = strategy.decode_str();
                    DVLOG(2) << "decode str: '" << str << "\\0'";
                    DVLOG(2) << "  ...'"
                        << ((ref > 0) ? decomp.str_at(ref) : ""_v)
                        << "' '"
                        << str
                        << "'";
                    decomp.decompress(ref, str, out);
                } else {
                    // rebuild the factorized string label
                    rebuilt_buffer.clear();

                    while (true) {
                        bool is_sub_char = !strategy.decode_sep();
                        DVLOG(2) << "decode sep: " << int(!is_sub_char);

                        if (is_sub_char) {
                            auto sub_chr = strategy.decode_char();
                            DVLOG(2) << "decode chr: " << int(sub_chr);

                            rebuilt_buffer.push_back(sub_chr);
                        } else {
                            auto sub_ref = strategy.decode_ref(Range(factor_count));
                            DVLOG(2) << "decode ref: " << sub_ref;

                            if (sub_ref == 0) {
                                // found a cut-value
                                auto cut = strategy.decode_ref(len_r);
                                DVLOG(2) << "decode special ref: " << cut;
                                while (cut > 0) {
                                    rebuilt_buffer.pop_back();
                                    --cut;
                                }
                            } else {
                                size_t prev_r = sub_ref;
                                size_t old_end = rebuilt_buffer.size();

                                // push the chars in reverse order
                                // to allow efficient appending of prefixes
                                do {
                                    View s = decomp.str_at(prev_r);
                                    prev_r = decomp.ref_at(prev_r);

                                    for (size_t j = 0; j < s.size(); j++) {
                                        rebuilt_buffer.push_back(s[s.size() - j - 1]);
                                    }
                                } while (prev_r != 0);
                                DCHECK_EQ(prev_r, 0);

                                // reverse suffix containing the new string fragment
                                std::reverse(rebuilt_buffer.begin() + old_end,
                                             rebuilt_buffer.end());
                            }
                        }

                        if (rebuilt_buffer.size() > 0 && rebuilt_buffer.back() == 0) {
                            rebuilt_buffer.pop_back();
                            break;
                        }
                    }
                    DVLOG(2) << "USED!";
                    DVLOG(2) << "  ...'"
                        << ((ref > 0) ? decomp.str_at(ref) : ""_v)
                        << "' '"
                        << rebuilt_buffer << "'";
                    decomp.decompress(ref, rebuilt_buffer, out);
                }

                /*
                std::cout << "in m (s,r): ("
                    << vec_to_debug_string(factor.string)
                    << ", " << int(factor.ref) << ")\n";
                */

                factor_count++;
            }
        }

        out << '\0';
        out.flush();
    }

    inline std::unique_ptr<Decompressor> decompressor() const override {
        return std::make_unique<WrapDecompressor>(*this);
    }
};


}//ns
