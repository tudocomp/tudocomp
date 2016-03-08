#ifndef INCLUDED_lzcics_lz78_hpp
#define INCLUDED_lzcics_lz78_hpp

#include "st.hpp"

namespace tudocomp {
namespace lz78 {
namespace lzcics {

constexpr static size_t undef = -1;

struct LZ78rule {
    std::vector<size_t> ref;
    std::vector<char> cha;
};

std::string lz78decode(const std::vector<size_t>& ref, const std::vector<char>& cha) {
    std::string t;
    for(size_t i = 0; i < ref.size(); ++i) {
        if(ref[i] == undef) {
            std::cout << "Rule: char " << cha[i] << std::endl;
            t += cha[i];
            continue;
        }
        std::cout << "Rule: ref " << ref[i] << ", cha: " << cha[i] << std::endl;

        std::string reff;
        size_t k = ref[i];
        while(k != undef) {
            reff = cha[k] + reff;
            k = ref[k];
        }
        std::cout << "Expand ref to " << reff << std::endl;
        t += reff + cha[i];
    }
    t.pop_back();
    return t;
}

/**
 * For LZ78 only:
 * Stores for each ST edge the number of letters explored by the LZ78 factorization
 * Each factor adds a new letter to 'ne', exploring an ST edge further by adding a new LZ trie node on the edge.
 */
struct ExplorationValues {
    const size_t nodes; //! number of internal nodes which might get explored by the LZ78 factorization
    size_t* ne; // storing the Exploration values
    /*
     * TODO:
     * Only internal nodes needed whose in-going edge has more than one letter.
     * n_e wastes a lot of memory
     *
     * TODO:
     * the factorization never explores nodes of ST whose string depth is larger than $\sqrt{2n}$.
     *
     * TODO:
     * Store only ne for the big nodes, for the small nodes, use the implicit marking
     *
     */
    ExplorationValues(const ST& st)
        : nodes(st.cst.bp.size()) //st.internal_nodes + st.cst.size()) // TODO: this value is too big
        , ne(new size_t[nodes])
    {
        std::cout << "Nodes-Size: " << nodes << std::endl;
        reset();
    }
    virtual ~ExplorationValues() {
        delete [] ne;
    }
    size_t operator[](const cst_t::node_type& node)const {
        assert(node < nodes);
        return ne[node];
    }
    void incr(const cst_t::node_type& node) {
        assert(node < nodes);
        std::cout << "ne incr " << node << std::endl;
        ++ne[node];
    }
    void reset() {
        for(size_t i = 0; i < nodes; ++i) ne[i] = 0;
    }

};

// size_t lcp(const std::string& a, const std::string& b) {
//     auto ait = a.begin();
//     auto bit = b.begin();
//     size_t ret = 0;
//     while( *(ait++) == *(bit++)) ++ret;
//     return ret;
// }

LZ78rule lz78naiv(const std::string& text, const ST&) {
    std::vector<size_t> ref;
    std::vector<char> cha;
    std::vector<std::string> factors;
    size_t refid = undef;
    size_t oldpos = 0;
    for(size_t i = 0; i < text.length()+1; ++i) {
        bool hasref = false;
        std::string sub = text.substr(oldpos, i-oldpos+1);
        std::cout << "examinging substr " << oldpos << " - " << (i-oldpos+1) << ":" << sub <<  std::endl;
        for(size_t f = 0; f < factors.size(); ++f) {
            if(factors[f] == sub) {
                refid = f;
                hasref = true;
                break;
            }
        }
        if(!hasref || i == text.length()) {
            ref.push_back(refid);
            cha.push_back(sub.back());
            factors.push_back(sub);
            oldpos = i+1;
            refid = undef;
        }
    }
        std::cout << "ref: ";
        for(size_t i = 0; i < ref.size(); ++i) std::cout << " " << ref[i];
        std::cout << std::endl;

        std::cout << "cha: ";
        for(size_t i = 0; i < cha.size(); ++i) std::cout << " " << cha[i];
        std::cout << std::endl;

        std::cout << "fac: ";
        for(size_t i = 0; i < factors.size(); ++i) std::cout << " " << factors[i];
        std::cout << std::endl;

    return LZ78rule { ref, cha };

}

LZ78rule lz78(const ST& st) {
    const cst_t& cst = st.cst;
    ExplorationValues ev(st);

    bit_vector bV(st.internal_nodes);
    bit_vector bC(st.internal_nodes);
    bit_vector bW(st.internal_nodes);
    auto ell = st.smallest_leaf();
    std::cout << "Selecting leaf " << ell << std::endl;
    do {
        auto v = st.root;
        size_t d = 0;
        do {
            v = st.level_anc(ell, ++d);
        } while(v != ell && bV[st.nid(v)] != 0);
        const auto u = st.parent(v);
        const size_t s = st.str_depth(u); //ev[u];
        if(v == ell) {
            std::cout << "Leaf-Access: Omitting " << s << "+1 chars" << std::endl;
            ell = st.next_mth_leaf(ell, s+1);
            std::cout << "Selecting leaf " << ell << std::endl;
            continue;
        }
        const auto vid = st.nid(v);
        bW[vid] = 1;

//      I do not get why this code does not work!!!
//
//         auto la = cst.leftmost_leaf(cst.select_child(v,st.child_rank(st.level_anc(ell,cst.node_depth(v)+1)) == 1 ? 0 : 1));
//         const size_t m = ev[v];
//         std::cout << "Omitting " << m << "+" << s << "+1 chars" << std::endl;
//         ell = st.next_mth_leaf(ell, m+s+1);
//         la = st.next_mth_leaf(la, m+s+1);
//         std::cout << "Selecting leaf " << ell << std::endl;
//         if(st.head(ell) != st.head(la)) {
//             bV[vid] = 1;
//         }
//         assert( (st.head(ell) != st.head(la)) == ([&v,&cst,&st,&m,&s] () {
//             // TODO: exchange la with l, use child-rank and level-anc to determine lb correctly
//             auto la = cst.leftmost_leaf(cst.select_child(v, 1));
//             auto lb = cst.leftmost_leaf(cst.select_child(v, 2));
//
//             std::cout << "Omitting " << m << "+" << s << "+1 chars" << std::endl;
// //            ell = st.next_mth_leaf(ell, m+s+1);
//             la = st.next_mth_leaf(la, m+s+1);
//             lb = st.next_mth_leaf(lb, m+s+1);
// //            std::cout << "Selecting leaf " << ell << std::endl;
//             return st.head(la) != st.head(lb);
//         })());


        // TODO: exchange la with l, use child-rank and level-anc to determine lb correctly
        auto la = cst.leftmost_leaf(cst.select_child(v, 1));
        auto lb = cst.leftmost_leaf(cst.select_child(v, 2));

        const size_t m = ev[v];
        std::cout << "Omitting " << m << "+" << s << "+1 chars" << std::endl;
        ell = st.next_mth_leaf(ell, m+s+1);
        la = st.next_mth_leaf(la, m+s+1);
        lb = st.next_mth_leaf(lb, m+s+1);
        std::cout << "Selecting leaf " << ell << std::endl;
        if(st.head(la) != st.head(lb)) {
            bV[vid] = 1;
        }
        ev.incr(v);
    } while(ell != st.smallest_leaf());
    std::cout << bV << std::endl;
    std::cout << bW << std::endl;
    std::cout << "Nodes:" << std::endl;
    for(size_t i = 0; i < ev.nodes; ++i) std::cout << ev[i] << std::endl;

    std::vector<size_t> pos;
    std::vector<size_t> len;

    rank_support_v5<1> rankW(&bW);
    const size_t zw = rankW.rank(bW.size());
    size_t W[zw]; // TODO: lower this to int_vector<Math.ceil(log(zw)/log(2))>
    for(size_t i = 0; i < zw; ++i) W[i] = undef;
    ev.reset();
    reset_bitvector(bV);
    size_t x = 0; // current factor == x-th factor

    std::vector<size_t> ref;
    std::vector<char> cha;

    do {
        auto v = st.root;
        size_t d = 0;
        do {
            v = st.level_anc(ell, ++d);
        } while(v != ell && bV[st.nid(v)] != 0);
        std::cout << "v: " << v << std::endl;
        const auto u = st.parent(v);
        const size_t s = st.str_depth(u); //ev[u];
        if(v == ell) {
            std::cout << "Leaf-Access: Omitting " << s << "+1 chars" << std::endl;
            ell = st.next_mth_leaf(ell, s);
            std::cout << "cha: " << st.head(ell) << std::endl;
            cha.push_back(st.head(ell));
            ell = st.next_leaf(ell);
            std::cout << "Selecting leaf " << ell << std::endl;
            ++x;
            if(cst.node_depth(v) == 1) {
                std::cout << "fresh factor" << std::endl;
                ref.push_back(undef);
            } else {
                const size_t z = W[rankW.rank(st.nid(st.parent(v)))];
                std::cout << x << "-th factor refers to " << z << "-th factor" << std::endl;
                ref.push_back(z);
            }
            continue;
        }
        const auto vid = st.nid(v);
        std::cout << "vid: " << vid << " witness: " << rankW.rank(vid) << std::endl;
        const auto y = W[rankW.rank(vid)];
        std::cout << "y= " << y << std::endl;
        if(y == undef) {
        std::cout << "depth(v)= " << cst.node_depth(v) << std::endl;
            if(cst.node_depth(v) == 1) {
                std::cout << "fresh factor" << std::endl;
                ref.push_back(undef);
            } else {
                const size_t z = W[rankW.rank(st.nid(st.parent(v)))];
                std::cout << x << "-th factor refers to " << z << "-th factor" << std::endl;
                ref.push_back(z);
            }
        }
        else {
            std::cout << x << "-th factor refers to " << y << "-th factor" << std::endl;
            ref.push_back(y);
        }

        W[rankW.rank(vid)] = x;
        std::cout << "W: ";
        for(size_t i = 0; i < zw; ++i) std::cout << " " << W[i];
        std::cout << std::endl;

        std::cout << "V: ";
        for(size_t i = 0; i < bV.size(); ++i) std::cout << " " << bV[i];
        std::cout << std::endl;

        std::cout << "N: ";
        for(size_t i = 0; i < ev.nodes; ++i) std::cout << " " << ev[i];
        std::cout << std::endl;


        // TODO: exchange la with l, use child-rank and level-anc to determine lb correctly
        auto la = cst.leftmost_leaf(cst.select_child(v, 1));
        auto lb = cst.leftmost_leaf(cst.select_child(v, 2));

        const size_t m = ev[v];
        std::cout << "Omitting " << m << "+" << s << "+1 chars" << std::endl;
        la = st.next_mth_leaf(la, m+s+1);
        lb = st.next_mth_leaf(lb, m+s+1);
        ell = st.next_mth_leaf(ell, m+s);
        std::cout << "cha: " << st.head(ell) << std::endl;
        cha.push_back(st.head(ell));
        ell = st.next_leaf(ell);
        std::cout << "Selecting leaf " << ell << std::endl;
        if(st.head(la) != st.head(lb)) {
            bV[vid] = 1;
        }
        ev.incr(v);
        ++x;
    } while(ell != st.smallest_leaf());

        std::cout << "ref: #" << ref.size() << " : ";
        for(size_t i = 0; i < ref.size(); ++i) std::cout << " " << ref[i];
        std::cout << std::endl;

        std::cout << "cha: " << cha.size() << " : ";
        for(size_t i = 0; i < cha.size(); ++i) std::cout << " " << cha[i];
        std::cout << std::endl;

    return LZ78rule { ref, cha };
}

}
}
}

#endif

