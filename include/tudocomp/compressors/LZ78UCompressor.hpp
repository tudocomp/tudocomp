#pragma once

#include <tudocomp/Compressor.hpp>

#include <sdsl/cst_fully.hpp>
#include <sdsl/cst_sada.hpp>

#include <tudocomp/Range.hpp>
#include <tudocomp/ds/TextDS.hpp>

#include <tudocomp/compressors/lz78/LZ78common.hpp>

#include "lz78u/SuffixTree.hpp"
#include "lz78u/HuffmanStringCoder.hpp"

namespace tdc {

// TODO: Define factorid for lz78u uniformly

namespace lz78u {
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

            std::cout << "    indices:         " << vec_to_debug_string(indices) << "\n";
            std::cout << "    start lit str:   " << vec_to_debug_string(start_literal_strings) << "\n";
            std::cout << "    literal_strings: " << vec_to_debug_string(literal_strings) << "\n";

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
            std::cout << "    reconstructed: " << vec_to_debug_string(buffer) << "\n";
            out << View(buffer);
        }

    };
}


template<typename coder_t, typename string_coder_t>
class LZ78UCompressor: public Compressor {
private:
    using node_type = SuffixTree::node_type;

public:
    inline LZ78UCompressor(Env&& env):
        Compressor(std::move(env))
    {}

    inline static Meta meta() {
        Meta m("compressor", "lz78u", "Lempel-Ziv 78 U\n\n" );
        m.option("coder").templated<coder_t, ASCIICoder>();
        m.option("string_coder").templated<string_coder_t, lz78u::HuffmanStringCoder>();
        // m.option("dict_size").dynamic("inf");
        m.needs_sentinel_terminator();
        return m;
    }

    virtual void compress(Input& input, Output& out) override {
        env().begin_stat_phase("lz78u");
        std::cout << "START COMPRESS\n";

        auto iview = input.as_view();
        View T = iview;

        SuffixTree::cst_t backing_cst;
        {
            env().begin_stat_phase("construct suffix tree");

            // TODO: Specialize sdsl template for less alloc here
            std::string bad_copy_1 = T.slice(0, T.size() - 1);
            std::cout << "text: " << vec_to_debug_string(bad_copy_1) << "\n";

            construct_im(backing_cst, bad_copy_1, 1);

            env().end_stat_phase();
        }
        SuffixTree ST(backing_cst);

        const size_t max_z = T.size() * bits_for(ST.cst.csa.sigma) / bits_for(T.size());
        env().log_stat("max z", max_z);

        sdsl::int_vector<> R(ST.internal_nodes,0,bits_for(max_z));

        len_t pos = 0;
        len_t z = 0;

        typedef SuffixTree::node_type node_t;

        typename coder_t::Encoder coder(env().env_for_option("coder"), out, NoLiterals());
        typename string_coder_t::Encoder string_coder(env().env_for_option("string_coder"), coder.stream());

        len_t factor_count = 0;

        auto output = [&](View slice, size_t ref) {
            // if trailing 0, remove
            if (slice.back() == 0) {
                slice = slice.substr(0, slice.size() - 1);
            }

            std::cout << "out (s,r): ("
                << vec_to_debug_string(slice)
                << ", " << int(ref) << ")" << std::endl;

            coder.encode(ref, Range(factor_count));
            string_coder.encode(slice);

            factor_count++;
        };

        // Skip the trailing 0
        while(pos < T.size() - 1) {
            const node_t l = ST.select_leaf(ST.cst.csa.isa[pos]);
            const len_t leaflabel = pos;

            if(ST.parent(l) == ST.root || R[ST.nid(ST.parent(l))] != 0) {
                const len_t parent_strdepth = ST.str_depth(ST.parent(l));

                std::cout << "out leaf: [" << (pos+parent_strdepth)  << ","<< (pos + parent_strdepth + 1) << "] ";
                output(T.slice(pos+parent_strdepth, pos + parent_strdepth + 1), R[ST.nid(ST.parent(l))]);

                pos += parent_strdepth+1;
                ++z;
                continue;
            }

            len_t d = 1;
            node_t parent = ST.root;
            node_t node = ST.level_anc(l, d);

            while(R[ST.nid(node)] != 0) {
                pos += ST.str_depth(node) - ST.str_depth(parent); // TODO: move outwards!
                parent = node;
                node = ST.level_anc(l, ++d);
            }

            R[ST.nid(node)] = ++z;

            const auto& str = T.slice(leaflabel + ST.str_depth(parent), leaflabel + ST.str_depth(node));

            std::cout << "out slice: [ "<< (leaflabel + ST.str_depth(parent)) << ", "<< (leaflabel + ST.str_depth(node))<< " ] ";
            output(str, R[ST.nid(ST.parent(node))]);

            pos += str.size();

        }

        env().end_stat_phase();
    }

    virtual void decompress(Input& input, Output& output) override final {
        std::cout << "START DECOMPRESS\n";
        auto out = output.as_stream();
        typename coder_t::Decoder decoder(env().env_for_option("coder"), input);
        typename string_coder_t::Decoder string_coder(env().env_for_option("string_coder"), decoder.stream());

        uint64_t factor_count = 0;

        lz78u::Decompressor decomp;
        std::vector<uliteral_t> buf;

        while (!decoder.eof()) {
            const lz78::factorid_t index = decoder.template decode<lz78::factorid_t>(Range(factor_count));

            buf.clear();
            string_coder.decode(buf);

            std::cout << "in m (s,r): ("
                << vec_to_debug_string(View(buf))
                << ", " << int(index) << ")\n";

            decomp.decompress(index, buf, out);

            factor_count++;
        }

        out << '\0';
        out.flush();
    }

};


}//ns
