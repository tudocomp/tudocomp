#pragma once

#include <tudocomp/Compressor.hpp>

#include <sdsl/cst_fully.hpp>
#include <sdsl/cst_sada.hpp>

#include <tudocomp/Range.hpp>
#include <tudocomp/ds/TextDS.hpp>

#include <tudocomp/ds/SuffixTree.hpp>
//#include <tudocomp/ds/st_util.hpp>

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

            std::cout << "start: " << start << ", end: " << end << "\n";
            std::cout << "literal_strings: " << ls << "\n";

            auto r1 = ls;
            DCHECK_LT(start, end);
            auto len = end - start;
            std::cout << "len: " << len << "\n";
            for (size_t xi = start; xi < start+len; xi++) {
                std::cout << int(ls[xi]) << "\n";
            }
            //View r2 = r1.substr(start, len);
            View r2 = View(&r1[start], len);

            std::cout << "slice r: " << vec_to_debug_string(r2) << "\n";

            return r2;
        }

        public:
        inline void decompress(lz78::factorid_t index, View literals, std::ostream& out) {
            indices.push_back(index);
            start_literal_strings.push_back(literal_strings.size());
            for (auto c : literals) {
                literal_strings.push_back(c);
            }

            std::cout << "indices:         " << vec_to_debug_string(indices) << "\n";
            std::cout << "start lit str:   " << vec_to_debug_string(start_literal_strings) << "\n";
            std::cout << "literal_strings: " << vec_to_debug_string(literal_strings) << "\n";

            buffer.clear();

            while(true) {
                std::cout << "a " << index << "\n";
                std::cout << "a " << vec_to_debug_string(buffer) << "\n";
                std::cout << "a " << vec_to_debug_string(literals) << "\n\n";

                auto inv_old_size = buffer.size();
                for (size_t i = 0; i < literals.size(); i++) {
                    buffer.push_back(literals[literals.size() - i - 1]);
                }

                DCHECK_GT(literals.size(), 0);
                DCHECK_EQ(inv_old_size + literals.size(), buffer.size());

                if (index == 0) {
                    break;
                }
                literals = literals_of(index - 1);
                index = indices[index - 1];
                std::cout << "b " << index << "\n";
                std::cout << "b " << vec_to_debug_string(buffer) << "\n";
                std::cout << "b " << vec_to_debug_string(literals) << "\n\n";
            }

            std::cout << "reconstructed: " << vec_to_debug_string(buffer) << "\n\n";

            for(size_t i = 0; i < buffer.size(); i++) {
                out << buffer[buffer.size() - i - 1];
            }
        }

    };
}


template<typename coder_t>
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
        // m.option("dict_size").dynamic("inf");
        m.needs_sentinel_terminator();
        return m;
    }

    virtual void compress(Input& input, Output& out) override {
        env().begin_stat_phase("lz78u");

        auto iview = input.as_view();
        View T = iview;

        SuffixTree::cst_t backing_cst;
        {
            env().begin_stat_phase("construct suffix tree");

            // TODO: Specialize sdsl template for less alloc here
            std::string bad_copy_1 = T.substr(0, T.size() - 1);
            std::cout << vec_to_debug_string(bad_copy_1) << "\n";

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

        len_t factor_count = 0;

        auto encode_factor_string = [&](View slice, typename coder_t::Encoder& coder) {
            for (auto c : slice) {
                coder.encode(c, literal_r);
            }
            coder.encode(0, literal_r);
        };

        auto output = [&](View slice, size_t ref) {
            std::cout << "out m (s,r): ("
                << vec_to_debug_string(slice)
                << ", " << int(ref) << ")\n";

            coder.encode(ref, Range(factor_count));
            encode_factor_string(slice, coder);

            factor_count++;
        };

        // Skip the trailing 0
        while(pos < T.size() - 1) {
            const node_t l = ST.select_leaf(ST.cst.csa.isa[pos]);
            const len_t leaflabel = pos;
            //std::cout << "Selecting leaf " << l << " with label " << leaflabel << std::endl;

            //std::cout << "Checking parent " << ST.parent(l) << " with R[parent] = " << R[ST.nid(ST.parent(l))] << std::endl;
            if(ST.parent(l) == ST.root || R[ST.nid(ST.parent(l))] != 0) {
//                DCHECK_EQ(T[pos + ST.str_depth(ST.parent(l))], lambda(ST.parent(l), l)[0]);

                auto i = pos + ST.str_depth(ST.parent(l));
                output(T.substr(i, i + 1), R[ST.nid(ST.parent(l))]);

                ++pos;
                ++z;
                continue;
            }
            len_t d = 1;
            node_t parent = ST.root;
            node_t node = ST.level_anc(l, d);
            while(R[ST.nid(node)] != 0) {
                //                    DCHECK_EQ(lambda(parent, node).size(), ST.str_depth(node) - ST.str_depth(parent));
                //                  pos += lambda(parent, node).size();
                pos += ST.str_depth(node) - ST.str_depth(parent);
                parent = node;
                node = ST.level_anc(l, ++d);
                //std::cout << "pos : " << pos << std::endl;
            }
            R[ST.nid(node)] = ++z;
            //std::cout << "Setting R[" << node << "] to " << z << std::endl;

            //std::cout << "Extracting substring for nodes (" << parent << ", " << node << ") " << std::endl;
            const auto& str = T.substr(leaflabel + ST.str_depth(parent), leaflabel + ST.str_depth(node));
            //std::cout << "extracted T[" << (leaflabel + ST.str_depth(parent)) << ", " << (leaflabel + ST.str_depth(node)) << "]: " << str << " of size " << str.size() << "\n";

            output(str, R[ST.nid(ST.parent(node))]);

            pos += str.size();
        }

        env().end_stat_phase();
    }

    virtual void decompress(Input& input, Output& output) override final {
        auto out = output.as_stream();
        typename coder_t::Decoder decoder(env().env_for_option("coder"), input);

        uint64_t factor_count = 0;

        lz78u::Decompressor decomp;
        std::vector<uliteral_t> buf;

        while (!decoder.eof()) {
            const lz78::factorid_t index = decoder.template decode<lz78::factorid_t>(Range(factor_count));

            buf.clear();
            while (true) {
                const uliteral_t chr = decoder.template decode<uliteral_t>(literal_r);
                if (chr == '\0') {
                    break;
                }
                buf.push_back(chr);
            }

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
