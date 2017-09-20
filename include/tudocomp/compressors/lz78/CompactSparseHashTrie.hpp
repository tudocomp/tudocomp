#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/lz78/LZ78Trie.hpp>
#include <tudocomp/compressors/lz78/squeeze_node.hpp>
#include <tudocomp/util/compact_sparse_hash.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {
namespace lz78 {


class CompactSparseHashTrie : public Algorithm, public LZ78Trie<factorid_t> {
    compact_hash<factorid_t> m_table;
    //std::unordered_map<uint64_t, factorid_t> m_table;

public:
    inline static Meta meta() {
        Meta m("lz78trie", "compact_sparse_hash", "Compact Sparse Hash Trie");
        //m.option("load_factor").dynamic(30);
        return m;
    }

    CompactSparseHashTrie(Env&& env, const size_t n, const size_t& remaining_characters, factorid_t reserve = 0)
        : Algorithm(std::move(env))
        , LZ78Trie(n,remaining_characters)
        , m_table(next_power_of_two(reserve))
    {
        //m_table.max_load_factor(this->env().option("load_factor").as_integer()/100.0f );
    }

    IF_STATS(
        MoveGuard m_guard;
        ~CompactSparseHashTrie() {
            if (m_guard) {
                //m_table.collect_stats(env());
            }
        }
    )
    CompactSparseHashTrie(CompactSparseHashTrie&& other) = default;
    CompactSparseHashTrie& operator=(CompactSparseHashTrie&& other) = default;

    node_t add_rootnode(uliteral_t c) override {
        auto key = create_node(0, c);
        auto value = size();

        m_table[key] = value;
        return value;
    }

    node_t get_rootnode(uliteral_t c) override {
        return c;
    }

    void clear() override {
        // m_table.clear();
    }

    node_t find_or_insert(const node_t& parent_w, uliteral_t c) override {
        auto parent = parent_w.id();

        // if we add a new node, its index will be equal to the current size of the dictionary
        const factorid_t newleaf_id = size();

        // Use 0 as special value for map lookup
        // if val == 0, then it got default constructed, which means
        // it is new
        // could also do it by the handler mechanism if this turns out to be a problem
        DCHECK_NE(newleaf_id, 0);

        auto key = create_node(parent,c);
        auto& val = m_table[key];
        if (val == 0) {
            val = newleaf_id;
            DCHECK_EQ(val, newleaf_id);
            return undef_id;
        } else {
            return val;
        }
    }

    factorid_t size() const override {
        return m_table.size();
    }
};

}} //ns
