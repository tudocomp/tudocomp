#pragma once

#include <tudocomp/Compressor.hpp>

#include <sdsl/cst_fully.hpp>
#include <sdsl/cst_sada.hpp>

#include <tudocomp/Range.hpp>
#include <tudocomp/ds/TextDS.hpp>

#include <tudocomp/ds/SuffixTree.hpp>
//#include <tudocomp/ds/st_util.hpp>

namespace tdc {

class LZ78UCompressor: public Compressor {
private:
    using node_type = SuffixTree::node_type;

public:
    inline LZ78UCompressor(Env&& env):
        Compressor(std::move(env))
    {}

    inline static Meta meta() {
        Meta m("compressor", "lz78u", "Lempel-Ziv 78 U\n\n" );
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

        // tx: a a a b a b a a a b a a b a b a $
        // sn:         5         10        15  16

        while(pos < T.size()) {
            const node_t l = ST.select_leaf(ST.cst.csa.isa[pos]);
            const len_t leaflabel = pos;
            std::cout << "Selecting leaf " << l << " with label " << leaflabel << std::endl;

            std::cout << "Checking parent " << ST.parent(l) << " with R[parent] = " << R[ST.nid(ST.parent(l))] << std::endl;
            if(ST.parent(l) == ST.root || R[ST.nid(ST.parent(l))] != 0) {
//                DCHECK_EQ(T[pos + ST.str_depth(ST.parent(l))], lambda(ST.parent(l), l)[0]);
                std::cout << "out l c: " << int(T[pos + ST.str_depth(ST.parent(l))]) << "\n";
                std::cout << "out l r: " << int(R[ST.nid(ST.parent(l))]) << "\n";
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
                std::cout << "pos : " << pos << std::endl;
            }
            R[ST.nid(node)] = ++z;
            std::cout << "Setting R[" << node << "] to " << z << std::endl;

            std::cout << "Extracting substring for nodes (" << parent << ", " << node << ") " << std::endl;
            const auto& str = T.substr(leaflabel + ST.str_depth(parent), leaflabel + ST.str_depth(node));
            std::cout << "extracted T[" << (leaflabel + ST.str_depth(parent)) << ", " << (leaflabel + ST.str_depth(node)) << "]: " << str << " of size " << str.size() << "\n";

            std::cout << "out m s: " << vec_to_debug_string(str) << "\n";
            std::cout << "out m r: " << int(R[ST.nid(ST.parent(node))]) << "\n";
            pos += str.size();
        }

        env().end_stat_phase();
    }

    virtual void decompress(Input& input, Output& output) override final {

    }

};


}//ns
