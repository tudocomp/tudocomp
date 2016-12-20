#pragma once

#include <tudocomp/Compressor.hpp>

#include <sdsl/cst_fully.hpp>

#include <tudocomp/Range.hpp>
#include <tudocomp/ds/TextDS.hpp>

namespace tdc {

class LZ78UCompressor: public Compressor {
private:
    using cst_t = sdsl::cst_sct3<>;
    using node_type = cst_t::node_type;

    static inline lz78::factorid_t select_size(Env& env, string_ref name) {
        auto& o = env.option(name);
        if (o.as_string() == "inf") {
            return 0;
        } else {
            return o.as_integer();
        }
    }

    /// Max dictionary size before reset
    const lz78::factorid_t m_dict_max_size {0};

public:
    inline LZ78UCompressor(Env&& env):
        Compressor(std::move(env)),
        m_dict_max_size(env.option("dict_size").as_integer())
    {}

    inline static Meta meta() {
        Meta m("compressor", "lz78u", "Lempel-Ziv 78 U\n\n" LZ78_DICT_SIZE_DESC);
        m.option("dict_size").dynamic("inf");
        m.needs_sentinel_terminator();
        return m;
    }

    virtual void compress(Input& input, Output& out) override {
        env().begin_stat_phase("lz78u");

        auto base_view = input.as_view();

        TextDS<> ds { base_view, env(), TextDS<>::ISA };
        auto isa_p = ds.release_isa();
        auto& ISA = *isa_p;

        auto T = base_view.substr(0, base_view.size() - 1);

        cst_t st;
        {
            env().begin_stat_phase("construct suffix tree");

            // TODO: Specialize sdsl template for less alloc here
            std::string bad_copy_1 = T;
            std::cout << vec_to_debug_string(bad_copy_1) << "\n";

            construct_im(st, bad_copy_1, 1);

            env().end_stat_phase();
        }

        std::vector<size_t> R;
        R.reserve(st.nodes());
        R.resize(st.nodes());

        size_t pos = 1;
        size_t parent_pos = 0;
        size_t z = 0;

        auto node = st.root();

        // tx: a a a b a b a a a b a a b a b a $
        // sn:         5         10        15  16

        while (pos <= T.size()) {
            node = st.child(node, T[pos]);

            bool is_leaf = st.is_leaf(node);
            if (is_leaf) {
                std::cout << "pos: " << pos << ", sn: " << st.sn(node) << "\n";
            }

            pos++;

            /*

            size_t node_idx = node;

            if (is_leaf) {
                std::cout << "out l char: " << vec_to_debug_string(T.substr(pos, pos + 1)) << "\n";
                std::cout << "out l idx:  " << 0 << "\n";
                node = st.root();
            } else if(R[node_idx] == 0) {
                z++;
                R[node_idx] = z;
                std::cout << "out m char: " << vec_to_debug_string(T.substr(pos, pos + 1)) << "\n";
                std::cout << "out m idx:  " << 0 << "\n";

            }

            pos += is_leaf ? 1 :*/
        }

        env().end_stat_phase();
    }

    virtual void decompress(Input& input, Output& output) override final {

    }

};


}//ns
