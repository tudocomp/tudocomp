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
	static_assert(std::numeric_limits<factorid_t>::radix == 2, "needs radix to be 2");

	typedef separate_chaining::group::group_chaining_table<> table_t;
    // typedef std::unordered_map<squeeze_node_t, factorid_t, _VignaHasher> table_t;

	
	static constexpr uint_fast8_t key_max_width = std::numeric_limits<factorid_t>::digits;
	static constexpr uint_fast8_t value_max_width = std::numeric_limits<squeeze_node_t>::digits;


	static constexpr size_t m_arraysize = 256;
	uint64_t m_array[m_arraysize];
	size_t m_elements = 0;

	static constexpr uint_fast8_t m_matrixrows = key_max_width - bits_for(m_arraysize);
	table_t* m_matrix[m_matrixrows];
	// table_t m_table;

public:
    inline static Meta meta() {
        Meta m(lz78_trie_type(), "chain", "Hash Trie with separate chaining");
        return m;
    }

    inline ChainingHashTrie(Config&& cfg, const size_t n, const size_t& remaining_characters, [[maybe_unused]] factorid_t reserve = 0)
        : Algorithm(std::move(cfg))
        , LZ78Trie(n, remaining_characters)
		// , m_table(64, std::numeric_limits<squeeze_node_t>::digits)
    {
        // if(reserve > 0) { m_table.reserve(reserve); }
		std::fill(m_array, m_array+m_arraysize, -1ULL);
		std::fill(m_matrix, m_matrix+m_matrixrows, nullptr);

    }
    IF_STATS(
        MoveGuard m_guard;
    )
		inline ~ChainingHashTrie() {
			clear();
    IF_STATS(
            if (m_guard) {
			// StatPhase logPhase;
			// m_table.print_stats(logPhase);
            }
    )
		}
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
		for(size_t i = 0; i < m_matrixrows; ++i) { 
			if(m_matrix[i] != nullptr) { 
				m_matrix[i]->clear(); 
				delete m_matrix[i];
			}
		}
		std::fill(m_matrix, m_matrix+m_matrixrows, nullptr);
		std::fill(m_array, m_array+m_arraysize, -1ULL);
		m_elements = 0;
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

		const squeeze_node_t large_key = key-m_arraysize;

		const uint_fast8_t large_key_width = bits_for(large_key);
		DCHECK_LE(large_key_width, m_matrixrows);
		if(m_matrix[large_key_width] == nullptr) {
			m_matrix[large_key_width] = new table_t(large_key_width-1, value_max_width);
		}
		const uint64_t subtract_key = (-1ULL>>(64-large_key_width+1));
		auto& table = *m_matrix[large_key_width];
		DCHECK_LT(subtract_key, large_key);
		DCHECK_LT(bits_for(large_key - subtract_key), large_key_width);
		const squeeze_node_t remaining_key = large_key - subtract_key;


		const size_t tablesize = table.size();
        auto it = table[remaining_key];

		DCHECK(tablesize == table.size() || tablesize+1 == table.size());

        if(tablesize+1 == table.size()) { // not found -> add a new node
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
