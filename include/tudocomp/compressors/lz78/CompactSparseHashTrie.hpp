#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/lz78/LZ78Trie.hpp>
#include <tudocomp/compressors/lz78/squeeze_node.hpp>
#include <tudocomp/util/compact_sparse_hash.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {
namespace lz78 {


class CompactSparseHashTrie : public Algorithm, public LZ78Trie<> {
    compact_hash<factorid_t> m_table;
    //std::unordered_map<uint64_t, factorid_t> m_table;
    size_t m_key_width = 0;

    inline size_t key_width(uint64_t key) {
        m_key_width = std::max(m_key_width, size_t(bits_for(key)));
        return m_key_width;
    }

public:
    inline static Meta meta() {
        Meta m("lz78trie", "compact_sparse_hash", "Compact Sparse Hash Trie");
        //m.option("load_factor").dynamic(30);
        return m;
    }

    inline CompactSparseHashTrie(Env&& env, const size_t n, const size_t& remaining_characters, factorid_t reserve = 0)
        : Algorithm(std::move(env))
        , LZ78Trie(n,remaining_characters)
        , m_table(zero_or_next_power_of_two(reserve), 0)
    {
        //m_table.max_load_factor(this->env().option("load_factor").as_integer()/100.0f );
    }

    IF_STATS(
        MoveGuard m_guard;
        inline ~CompactSparseHashTrie() {
            if (m_guard) {
                //m_table.collect_stats(env());
            }
        }
    )
    CompactSparseHashTrie(CompactSparseHashTrie&& other) = default;
    CompactSparseHashTrie& operator=(CompactSparseHashTrie&& other) = default;

    inline node_t add_rootnode(uliteral_t c) {
        auto key = create_node(0, c);
        auto value = size();

        auto& entry = m_table.index(key, key_width(key));

        //std::cout << "find_or_insert(" << key << ", " << entry << ", " << value << ");\n";

        entry = value;
        return node_t(value, true);
    }

    inline node_t get_rootnode(uliteral_t c) const {
        return node_t(c, false);
    }

    inline void clear() {
        // m_table.clear();
    }

    inline node_t find_or_insert(const node_t& parent_w, uliteral_t c) {
        auto parent = parent_w.id();

        // if we add a new node, its index will be equal to the current size of the dictionary
        const factorid_t newleaf_id = size();

        // Use 0 as special value for map lookup
        // if val == 0, then it got default constructed, which means
        // it is new
        // could also do it by the handler mechanism if this turns out to be a problem
        DCHECK_NE(newleaf_id, 0);

        auto key = create_node(parent,c);
        auto& val = m_table.index(key, key_width(key));
        if (val == 0) {
            val = newleaf_id;
            DCHECK_EQ(val, newleaf_id);
            //std::cout << "find_or_insert(" << key << ", " << val << ", " << newleaf_id << ");\n";
            return node_t(val, true);
        } else {
            //std::cout << "find_or_insert(" << key << ", " << val << ", " << val << ");\n";
            return node_t(val, false);
        }
    }

    inline size_t size() const {
        return m_table.size();
    }
};

}} //ns
