#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/util/Hash.hpp>
#include <tudocomp/compressors/lz78/LZ78Trie.hpp>
#include <tudocomp/compressors/lz78/squeeze_node.hpp>
#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {
namespace lz78 {


template<class HashFunction = MixHasher, class HashProber = LinearProber, class HashManager = SizeManagerPow2>
class HashTrie : public Algorithm, public LZ78Trie<factorid_t> {
    HashMap<squeeze_node_t,factorid_t,undef_id,HashFunction,std::equal_to<squeeze_node_t>,HashProber,HashManager> m_table;

public:
    inline static Meta meta() {
        Meta m("lz78trie", "hash", "Hash Trie");
        m.option("hash_function").templated<HashFunction, MixHasher>("hash_function");
        m.option("hash_prober").templated<HashProber, LinearProber>("hash_prober");
        m.option("hash_manager").templated<HashManager, SizeManagerPow2>("hash_manager");
        m.option("load_factor").dynamic(30);
        return m;
    }

    inline HashTrie(Env&& env, const size_t n, const size_t& remaining_characters, factorid_t reserve = 0)
        : Algorithm(std::move(env))
        , LZ78Trie(n,remaining_characters)
        , m_table(this->env(),n,remaining_characters)
    {
        m_table.max_load_factor(this->env().option("load_factor").as_integer()/100.0f );
        if(reserve > 0) {
            m_table.reserve(reserve);
        }
    }

    IF_STATS(
        MoveGuard m_guard;
        inline ~HashTrie() {
            if (m_guard) {
                m_table.collect_stats(env());
            }
        }
    )
    HashTrie(HashTrie&& other) = default;
    HashTrie& operator=(HashTrie&& other) = default;

    inline node_t add_rootnode(uliteral_t c) {
        m_table.insert(
            std::make_pair<squeeze_node_t,factorid_t>(
                create_node(0, c),
                size()
            )
        );
        return node_t(size() - 1, true);
    }

    inline node_t get_rootnode(uliteral_t c) const {
        return node_t(c, false);
    }

    inline void clear() {
//        table.clear();

    }

    inline node_t find_or_insert(const node_t& parent_w, uliteral_t c) {
        auto parent = parent_w.id();
        const factorid_t newleaf_id = size(); //! if we add a new node, its index will be equal to the current size of the dictionary

        auto ret = m_table.insert(
            std::make_pair(
                create_node(parent,c),
                newleaf_id
            )
        );
        if(ret.second) return node_t(newleaf_id, true); // added a new node
        return node_t(ret.first.value(), false);
    }

    inline size_t size() const {
        return m_table.entries();
    }
};

}} //ns

