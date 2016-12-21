#pragma once

#include <tudocomp/Compressor.hpp>

#include <sdsl/cst_fully.hpp>
#include <sdsl/cst_sada.hpp>

#include <tudocomp/Range.hpp>
#include <tudocomp/ds/TextDS.hpp>

namespace tdc {

class LZ78UCompressor: public Compressor {
private:
    using cst_t = sdsl::cst_sada<>;
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

        auto T = input.as_view();

        TextDS<> ds { T, env(), TextDS<>::ISA };
        auto isa_p = ds.release_isa();
        auto& ISA = *isa_p;

        cst_t ST;
        {
            env().begin_stat_phase("construct suffix tree");

            // TODO: Specialize sdsl template for less alloc here
            std::string bad_copy_1 = T.substr(0, T.size() - 1);
            std::cout << vec_to_debug_string(bad_copy_1) << "\n";

            construct_im(ST, bad_copy_1, 1);

            env().end_stat_phase();
        }

        std::vector<size_t> R;
        R.reserve(ST.nodes());
        R.resize(ST.nodes());

        size_t pos = 1;
        size_t z = 0;

        // tx: a a a b a b a a a b a a b a b a $
        // sn:         5         10        15  16

        auto lambda = [&ST, &pos, &T](const node_type& l1, const node_type& l2) -> View {
            //auto size1 = ST.depth(l1);
            //auto size2 = ST.depth(l2);
            auto offset1 = T.size() - ST.depth(l1);
            auto offset2 = T.size() - ST.depth(l2);

            std::cout << "pos: " << pos << " lambda: " << offset1 << " vs " << offset2 << "\n";

            //auto offset1 = ST.sn(l1);
            //auto offset2 = ST.sn(l2);

            if (offset1 < offset2) {
                return T.substr(offset1, offset2);
            } else {
                throw std::runtime_error("asdf");
            }
        };

        auto parent = [&ST](const node_type& n) {
            return ST.parent(n);
        };

        auto leaf_select = [&ST](const size_t& pos) {
            return ST.select_leaf(pos);
        };

        auto level_anc = [&ST](const node_type& n, size_t d) {
            return ST.sl(n, d);
        };

        while (pos <= T.size()) {
            auto l = leaf_select(ISA[pos]);

            if (R[parent(l)] != 0) {
                std::cout << "out l c: " << int(lambda(parent(l), l)[0]) << "\n";
                std::cout << "out l r: " << int(0) << "\n";
                pos++;
            } else {
                size_t d = 1;
                while (level_anc(l, d) != 0) {
                    d++;
                    pos += lambda(level_anc(l, d - 1), level_anc(l, d)).size();
                }
                auto node = level_anc(l, d);
                z++;
                R[node] = z;

                std::cout << "out m s: " << vec_to_debug_string(lambda(parent(node), node)) << "\n";
                std::cout << "out m r: " << int(R[parent(node)]) << "\n";
                pos += lambda(parent(node), node).size();
            }
        }

        env().end_stat_phase();
    }

    virtual void decompress(Input& input, Output& output) override final {

    }

};


}//ns
