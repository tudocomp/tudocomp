#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/lz78/LZ78Trie.hpp>
#include <tudocomp/compressors/lz78/squeeze_node.hpp>
#include <tudocomp/util/Hash.hpp>
#include <tudocomp_stat/StatPhase.hpp>

#include <unordered_map>

namespace tdc {
namespace lz78 {

class ExtHashTrie : public Algorithm, public LZ78Trie<> {
    typedef std::unordered_map<squeeze_node_t, factorid_t, _VignaHasher> table_t;
//    typedef rigtorp::HashMap<squeeze_node_t, factorid_t,CLhash> table_t;
//    typedef ska::flat_hash_map<squeeze_node_t, factorid_t,CLhash> table_t; //ska::power_of_two_std_hash<size_t>> table_t;
//    typedef ska::flat_hash_map<squeeze_node_t, factorid_t,ska::power_of_two_std_hash<size_t>> table_t;
//    typedef rigtorp::HashMap<squeeze_node_t, factorid_t,_VignaHasher> table_t;

    table_t m_table;
  const size_t m_n;
  const size_t& m_remaining_characters;

public:
    inline static Meta meta() {
        Meta m(lz78_trie_type(), "exthash", "Hash Trie with external hash table");
        return m;
    }

    inline ExtHashTrie(Config&& cfg, const size_t n, const size_t& remaining_characters, factorid_t reserve = 0)
        : Algorithm(std::move(cfg))
        , LZ78Trie(n, remaining_characters)
        , m_n(n)
        , m_remaining_characters(remaining_characters)
    {
    //    m_table.max_load_factor(0.9f);
        if(reserve > 0) {
            m_table.reserve(reserve);
        }
    }
    // ExtHashTrie(Env&& env, factorid_t reserve = 0) : Algorithm(std::move(env)) {
    //     if(reserve > 0) {
    //         m_table.reserve(reserve);
    //     }
    // }
    IF_STATS(
        MoveGuard m_guard;
        inline ~ExtHashTrie() {
            if (m_guard) {
                StatPhase::log("table size", m_table.bucket_count());
                StatPhase::log("load factor", m_table.max_load_factor());
                StatPhase::log("entries", m_table.size());
                StatPhase::log("load ratio", m_table.load_factor());
            }
        }
    )
    ExtHashTrie(ExtHashTrie&& other) = default;
    ExtHashTrie& operator=(ExtHashTrie&& other) = default;

    inline node_t add_rootnode(uliteral_t c) {
        m_table.insert(std::make_pair<squeeze_node_t,factorid_t>(create_node(0, c), size()));
        return node_t(size() - 1, true);
    }

    inline node_t get_rootnode(uliteral_t c) const {
        return node_t(c, false);
    }

    inline void clear() {
        m_table.clear();

    }

    inline node_t find_or_insert(const node_t& parent_w, uliteral_t c) {
        auto parent = parent_w.id();
        const factorid_t newleaf_id = size(); //! if we add a new node, its index will be equal to the current size of the dictionary

        auto ret = m_table.insert(std::make_pair(create_node(parent+1,c), newleaf_id));

        if(ret.second) { // added a new node
            if(tdc_unlikely(m_table.bucket_count()*m_table.max_load_factor() < m_table.size()+1)) {
                const size_t expected_size = (m_table.size() + 1 + lz78_expected_number_of_remaining_elements(m_table.size(), m_n, m_remaining_characters))/0.95;
                if(expected_size < m_table.bucket_count()*2.0*0.95) {
                    m_table.reserve(expected_size);
                }
            }
            return node_t(size() - 1, true); // added a new node
        }

        return node_t(ret.first->second, false); // return the factor id of that node
    }

    inline size_t size() const {
        return m_table.size();
    }
};

}} //ns

