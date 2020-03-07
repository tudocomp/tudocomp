#pragma once
//! uses the compact hash table implementations of https://github.com/koeppl/separate_chaining

#ifndef STATS_DISABLED
	#define STATS_ENABLED
#endif

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/lz78/LZ78Trie.hpp>
#include <tudocomp/compressors/lz78/squeeze_node.hpp>
#include <separate/group_chaining.hpp>
#include <tudocomp/util.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {
namespace lz78 {


class ChainingHashTrie : public Algorithm, public LZ78Trie<> {
	static_assert(std::numeric_limits<squeeze_node_t>::radix == 2, "needs radix to be 2");

	typedef separate_chaining::group::group_chaining_table<> table_t;
    // typedef std::unordered_map<squeeze_node_t, factorid_t, _VignaHasher> table_t;

	table_t m_table;

	static constexpr size_t m_arraysize = 256;
	uint64_t m_array[m_arraysize];
	size_t m_elements = 0;

public:
    inline static Meta meta() {
        Meta m(lz78_trie_type(), "chain", "Hash Trie with separate chaining");
        return m;
    }

    inline ChainingHashTrie(Config&& cfg, const size_t n, const size_t& remaining_characters, factorid_t reserve = 0)
        : Algorithm(std::move(cfg))
        , LZ78Trie(n, remaining_characters)
		, m_table(64, std::numeric_limits<squeeze_node_t>::digits)
    {
        if(reserve > 0) { m_table.reserve(reserve); }
		std::fill(m_array, m_array+m_arraysize, -1ULL);
    }
    IF_STATS(
        MoveGuard m_guard;
        inline ~ChainingHashTrie() {
            if (m_guard) {
			StatPhase logPhase;
			m_table.print_stats(logPhase);
            }
        }
    )
    ChainingHashTrie(ChainingHashTrie&& other) = default;
    ChainingHashTrie& operator=(ChainingHashTrie&& other) = default;

    inline node_t add_rootnode(uliteral_t c) {
		DCHECK_LT(create_node(0, c), m_arraysize);
		m_array[create_node(0, c)] = size();
		++m_elements;
		// m_table[create_node(0, c)] = size();
        return node_t(size() - 1, true);
    }

    inline node_t get_rootnode(uliteral_t c) const {
        return node_t(c, false);
    }

    inline void clear() {
        m_table.clear();
		m_elements = 0;
		std::fill(m_array, m_array+m_arraysize, -1ULL);
    }

    inline node_t find_or_insert(const node_t& parent_w, uliteral_t c) {
        auto parent = parent_w.id();
        const factorid_t newleaf_id = size(); //! if we add a new node, its index will be equal to the current size of the dictionary

		const squeeze_node_t key = create_node(parent+1,c);
		if(key < m_arraysize) {
			if(m_array[key] == -1ULL) { // not found -> add a new node
				++m_elements;
				m_array[key] = newleaf_id;
				return node_t(size() - 1, true); 
			}
			return node_t(m_array[key], false); // return the factor id of that node
		}
		// bits_for(key-m_arraysize)

		const size_t tablesize = m_table.size();
        auto it = m_table[create_node(parent+1,c)];

		DCHECK(tablesize == m_table.size() || tablesize+1 == m_table.size());

        if(tablesize+1 == m_table.size()) { // not found -> add a new node
			++m_elements;
			it = newleaf_id;
			DCHECK_EQ(it.value(), newleaf_id);
            return node_t(size() - 1, true); 
        }

        return node_t(it.value(), false); // return the factor id of that node
    }

    inline size_t size() const {
        return m_elements;
    }
};


}} //ns
