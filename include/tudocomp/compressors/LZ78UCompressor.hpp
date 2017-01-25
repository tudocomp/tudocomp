#pragma once

#include <tudocomp/Compressor.hpp>

#include <sdsl/cst_fully.hpp>
#include <sdsl/cst_sada.hpp>

#include <tudocomp/Range.hpp>
#include <tudocomp/ds/TextDS.hpp>

#include <tudocomp/compressors/lz78/LZ78common.hpp>

#include "lz78u/SuffixTree.hpp"

#include "lz78u/pre_header.hpp"

namespace tdc {
namespace lz78u {

    // TODO: Define factorid for lz78u uniformly

    class Decompressor {
        std::vector<lz78::factorid_t> indices;
        std::vector<uliteral_t> literal_strings;
        std::vector<size_t> start_literal_strings;

        std::vector<uliteral_t> buffer;

        public:

        inline View literals_of(size_t i) const {
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

        inline void output_str(View str, std::ostream& out) {
            out << str;
        }

        inline void decompress(lz78::factorid_t index, View literals, std::ostream& out) {
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
                literals = literals_of(index - 1);
                index = indices[index - 1];
            }

            std::reverse(buffer.begin(), buffer.end());
            //std::cout << "    reconstructed: " << vec_to_debug_string(buffer) << "\n";
            output_str(buffer, out);
        }

    };
}

template<typename strategy_t, typename ref_coder_t>
class LZ78UCompressor: public Compressor {
private:
    using node_type = SuffixTree::node_type;

    using RefEncoder = typename ref_coder_t::Encoder;
    using RefDecoder = typename ref_coder_t::Decoder;

    using CompressionStrat
        = typename strategy_t::template Compression<RefEncoder>;
    using DecompressionStrat
        = typename strategy_t::template Decompression<RefDecoder>;

public:
    inline LZ78UCompressor(Env&& env):
        Compressor(std::move(env))
    {}

    inline static Meta meta() {
        Meta m("compressor", "lz78u", "Lempel-Ziv 78 U\n\n" );
        m.option("strategy").templated<strategy_t>();
        m.option("coder").templated<ref_coder_t>();
        m.option("threshold").dynamic("3");
        // m.option("dict_size").dynamic("inf");
        m.needs_sentinel_terminator();
        return m;
    }

    virtual void compress(Input& input, Output& out) override {
        auto phase1 = env().stat_phase("lz78u");
        //std::cout << "START COMPRESS\n";
        const len_t threshold = env().option("threshold").as_integer(); //factor threshold
        env().log_stat("threshold", threshold);

        auto iview = input.as_view();
        View T = iview;

        SuffixTree::cst_t backing_cst;
        {
            auto phase2 = env().stat_phase("construct suffix tree");

            // TODO: Specialize sdsl template for less alloc here
            std::string bad_copy_1 = T.slice(0, T.size() - 1);
            //std::cout << "text: " << vec_to_debug_string(bad_copy_1) << "\n";

            construct_im(backing_cst, bad_copy_1, 1);
        }
        SuffixTree ST(backing_cst);

        const size_t max_z = T.size() * bits_for(ST.cst.csa.sigma) / bits_for(T.size());
        env().log_stat("max z", max_z);

        sdsl::int_vector<> R(ST.internal_nodes,0,bits_for(max_z));

        len_t pos = 0;
        len_t z = 0;

        typedef SuffixTree::node_type node_t;

        CompressionStrat strategy {
            env().env_for_option("strategy"),
            env().env_for_option("coder"),
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
                        if(pos >= end) {
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
            } else {
                // else just output the string as a whole

                // indicate that this is not a factorized string
                DVLOG(2) << "plain";
                strategy.encode_sep(true);
                strategy.encode_str(T.slice(begin,end));
            }

            factor_count++;
        };

        // Skip the trailing 0
        while(pos < T.size() - 1) {
            const node_t l = ST.select_leaf(ST.cst.csa.isa[pos]);
            const len_t leaflabel = pos;

            if(ST.parent(l) == ST.root || R[ST.nid(ST.parent(l))] != 0) {
                const len_t parent_strdepth = ST.str_depth(ST.parent(l));

                //std::cout << "out leaf: [" << (pos+parent_strdepth)  << ","<< (pos + parent_strdepth + 1) << "] ";
                output(pos+parent_strdepth, pos + parent_strdepth + 1, R[ST.nid(ST.parent(l))]);

                pos += parent_strdepth+1;
                ++z;
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
            output(begin,end, R[ST.nid(ST.parent(node))]);
            R[ST.nid(node)] = ++z;

            pos += end - begin;

        }
    }

    virtual void decompress(Input& input, Output& output) override final {
        //std::cout << "START DECOMPRESS\n";
        auto out = output.as_stream();

        {
            DecompressionStrat strategy {
                env().env_for_option("strategy"),
                env().env_for_option("coder"),
                std::make_shared<BitIStream>(input)
            };

            uint64_t factor_count = 0;

            lz78u::Decompressor decomp;

            std::vector<uliteral_t> rebuilt_buffer;

            while (!strategy.eof()) {
                auto ref = strategy.decode_ref(Range(factor_count));
                bool not_factorized = strategy.decode_sep();

                if (not_factorized) {
                    auto str = strategy.decode_str();
                    decomp.decompress(ref, str, out);
                } else {
                    // rebuild the factorized string label
                    rebuilt_buffer.clear();

                    while (true) {
                        bool is_sub_char = !strategy.decode_sep();

                        if (is_sub_char) {
                            auto sub_chr = strategy.decode_char();

                            rebuilt_buffer.push_back(sub_chr);
                        } else {
                            auto sub_ref = strategy.decode_ref(Range(factor_count));
                            if (sub_ref == 0) {
                                // found a cut-value
                                auto cut = strategy.decode_ref(len_r);
                                while (cut > 0) {
                                    rebuilt_buffer.pop_back();
                                    --cut;
                                }
                            } else {
                                auto s = decomp.literals_of(sub_ref - 1);
                                for (auto c : s) {
                                    rebuilt_buffer.push_back(c);
                                }
                            }
                        }

                        if (rebuilt_buffer.size() > 0 && rebuilt_buffer.back() == 0) {
                            break;
                        }
                    }
                    decomp.output_str(rebuilt_buffer, out);
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

};


}//ns
