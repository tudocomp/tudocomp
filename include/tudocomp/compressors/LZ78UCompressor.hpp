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

        View literals_of(size_t i) const {
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

        public:
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
            out << View(buffer);
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

        auto output = [&](len_t begin, len_t end, size_t ref) { // end is the position after the substring, i.e., T[begin..e] where e= end-1
            // if trailing 0, remove
            while(T[end-1] == 0) --end;

            // factorize the factor label if the label is above the threshold
            if(end-begin >= threshold ) {
                std::vector<len_t> refs; // stores either referred indices or characters
                std::vector<bool> is_ref; // stores whether refs stores at a position a index or a character
                for(len_t pos = begin; pos < end;) { // similar to the normal LZ78U factorization, but does not introduce new factor ids
                    const node_t leaf = ST.select_leaf(ST.cst.csa.isa[pos]);
                    len_t d = 1;
                    node_t parent = ST.root;
                    node_t node = ST.level_anc(leaf, d);
                    while(!ST.cst.is_leaf(node) && R[ST.nid(node)] != 0) {
                        parent = node;
                        node = ST.level_anc(leaf, ++d);
                    } // not a good feature: We lost the factor ids of the leaves, since R only stores the IDs of internal nodes
                    DCHECK_NE(parent, ST.root);
                    const len_t depth = ST.str_depth(parent);
                    // if the largest factor is not large enough, we only store the current character and move one text position to the right
                    if(depth < threshold) {
                        refs.push_back(T[pos]);
                        is_ref.push_back(false); // the current ref is a plain character
                        ++pos;
                    } else {
                        refs.push_back(R[ST.nid(parent)]);
                        is_ref.push_back(true); // the current ref is a real ref
                        pos += depth;
                        // taking the last factor can make the string larger than intended, so we have to store a cut-value at the last position
                        if(pos >= end) {
                            refs.push_back(pos-end); // we do not store in is_ref something such that we know later that the last value is a cutting value
                        }
                    }
                }

                /*
                // trying to rebuild the factorized string label
                std::string rebuilt;
                for(len_t i = 0; i < refs.size(); ++i) {
                    if(!is_ref[i]) {
                        rebuilt.push_back(refs[i]);
                        continue;
                    }
                    DCHECK_NE(refs[i],0);
                    for(auto it = ST.cst.begin(); it != ST.cst.end(); ++it) { // we have to query the inverse of R -> this takes time!
                        if(ST.cst.is_leaf(*it)) continue;
                        if(R[ST.nid(*it)] == refs[i]) { // if we found the right factor in the suffix tree, we can decode the factor
                            const node_t node = *it;
                            const len_t suffix_number = ST.cst.sn(ST.cst.leftmost_leaf(node));
                            const len_t depth = ST.str_depth(node);
                            rebuilt += T.slice(suffix_number, suffix_number + depth);
                            DCHECK_EQ(T.slice(begin, begin+rebuilt.size()), rebuilt);
                            break;
                        }
                    }
                    if(i+2 == refs.size() && is_ref.size() < refs.size()) { // there is an offset at the last position of refs
                        while(refs[i+1] > 0) {
                            rebuilt.pop_back();
                            --refs[i+1];
                        }
                        break;
                    }
                }
                DCHECK_EQ(rebuilt.size(), end-begin);
                DCHECK_EQ(rebuilt, T.slice(begin,end));

                */

            } else {
                strategy.encode(lz78u::Factor { T.slice(begin,end), ref }, factor_count);
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

            R[ST.nid(node)] = ++z;

            // const auto& str = T.slice(leaflabel + ST.str_depth(parent), leaflabel + ST.str_depth(node));
            const len_t begin = leaflabel + ST.str_depth(parent);
            const len_t end = leaflabel + ST.str_depth(node);

            //std::cout << "out slice: [ "<< (leaflabel + ST.str_depth(parent)) << ", "<< (leaflabel + ST.str_depth(node))<< " ] ";
            output(begin,end, R[ST.nid(ST.parent(node))]);

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

            while (!strategy.eof()) {
                auto factor = strategy.decode(factor_count);

                /*
                std::cout << "in m (s,r): ("
                    << vec_to_debug_string(factor.string)
                    << ", " << int(factor.ref) << ")\n";
                */

                decomp.decompress(factor.ref, factor.string, out);

                factor_count++;
            }
        }

        out << '\0';
        out.flush();
    }

};


}//ns
