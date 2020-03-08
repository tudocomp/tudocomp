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

	
	static constexpr uint_fast8_t value_max_width = std::numeric_limits<factorid_t>::digits;
	static constexpr uint_fast8_t key_max_width = std::numeric_limits<squeeze_node_t>::digits;


	static constexpr size_t m_arraysize = 256;
	factorid_t m_array[m_arraysize];
	size_t m_elements = 0;

	static constexpr size_t m_smalltable_key_width = 9;
	table_t m_smalltable;

	static constexpr uint_fast8_t m_rowlength = 10; 
	table_t* m_rows[m_rowlength];

	static constexpr uint_fast8_t m_matrixlength = key_max_width - m_rowlength - m_smalltable_key_width - bits_for(m_arraysize);
	table_t** m_matrix[m_rowlength];

public:
    inline static Meta meta() {
        Meta m(lz78_trie_type(), "chain", "Hash Trie with separate chaining");
        return m;
    }

    inline ChainingHashTrie(Config&& cfg, const size_t n, const size_t& remaining_characters, [[maybe_unused]] factorid_t reserve = 0)
        : Algorithm(std::move(cfg))
        , LZ78Trie(n, remaining_characters)
		, m_smalltable(m_smalltable_key_width, value_max_width)
    {
        // if(reserve > 0) { m_table.reserve(reserve); }
		std::fill(m_array, m_array+m_arraysize, -1ULL);
		std::fill(m_rows, m_rows+m_rowlength, nullptr);
		std::fill(m_matrix, m_matrix+m_matrixlength, nullptr);

    }
    IF_STATS(
        MoveGuard m_guard;
    )
		inline ~ChainingHashTrie() {
    // IF_STATS(
            // if (m_guard) {
				size_t array_filled = 0;
				for(size_t i = 0; i < m_arraysize; ++i) {
					if(m_array[i] == static_cast<factorid_t>(-1ULL)) { continue; }
					++array_filled;
				}
				StatPhase::log("array_fill", array_filled);
				if(m_smalltable.size() > 0) {
					StatPhase::log("small_table_size", m_smalltable.size());
				}
				
				for(size_t i = 0; i < m_rowlength; ++i) {
					if(m_rows[i] == nullptr) { continue; }
					uint64_t value_widths[65];
					std::fill(value_widths, value_widths+65, 0);
					for(auto it = m_rows[i]->begin(); it != m_rows[i]->end(); ++it) {
						++value_widths[bits_for(it.value())];
					}
					for(size_t k = 0; k <= 64; ++k) {
						std::stringstream s;
						s << "r[" << std::setw(2) << i << "][" << std::setw(2) <<  k << "]";
						std::string strkey = s.str();
						if(value_widths[k] > 0) {
							StatPhase::log(std::move(strkey), value_widths[k]);
						}
					}
				}
				for(size_t i = 0; i < m_matrixlength; ++i) {
					if(m_matrix[i] == nullptr) { continue; }
					for(size_t k = 0; k < value_max_width; ++k) {
						if(m_matrix[i][k] == nullptr) { continue; }
						std::stringstream s;
						s << "m[" << std::setw(2) << i << "][" << std::setw(2) <<  k << "]";
						std::string strkey = s.str();
						StatPhase::log(std::move(strkey), m_matrix[i][k]->size());
					}
				}
			// StatPhase logPhase;
			// m_table.print_stats(logPhase);
            // }
    // )//STATS
			clear();
		}
    ChainingHashTrie(ChainingHashTrie&& other) = default;
    ChainingHashTrie& operator=(ChainingHashTrie&& other) = default;

    inline node_t add_rootnode(uliteral_t c) { //TODO: optimize -> seems that we do not need to store anything in m_array!
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
		for(size_t i = 0; i < m_matrixlength; ++i) { 
			if(m_matrix[i] == nullptr) { continue; }
			for(size_t j = 0; j < value_max_width; ++j) {
				if(m_matrix[i][j] != nullptr) {
					delete m_matrix[i][j];
				}
			}
			delete [] m_matrix[i];
		}
		for(size_t i = 0; i < m_rowlength; ++i) { 
			if(m_rows[i] == nullptr) { continue; }
			m_rows[i]->clear();  //TODO: not needed?
			delete m_rows[i];
			m_rows[i] = nullptr;
		}

		std::fill(m_matrix, m_matrix+m_matrixlength, nullptr);
		std::fill(m_rows, m_rows+m_rowlength, nullptr);
		std::fill(m_array, m_array+m_arraysize, -1ULL);
		m_elements = 0;
		m_smalltable.clear();
    }

    inline node_t find_or_insert(const node_t& parent_w, uliteral_t c) {
        auto parent = parent_w.id();
        const factorid_t newleaf_id = size(); //! if we add a new node, its index will be equal to the current size of the dictionary

		const squeeze_node_t key = create_node(parent+1,c);
		if(key < m_arraysize) {
			if(m_array[key] == static_cast<factorid_t>(-1ULL)) { //! not found -> add a new node
				++m_elements;
				m_array[key] = newleaf_id;
				return node_t(size() - 1, true); 
			}
			return node_t(m_array[key], false); //! return the factor id of that node
		}

		const squeeze_node_t large_key = key-m_arraysize; //! since we know that the key is larger than `m_arraysize`, we can subtract this value from the key we want to store
		const uint_fast8_t large_key_width = bits_for(large_key);
		if(large_key_width < m_smalltable_key_width) { //! use the small table
			const size_t tablesize = m_smalltable.size();
			DCHECK_LE(large_key_width, m_smalltable.key_width());
			auto it = m_smalltable[large_key];
			DCHECK(tablesize == m_smalltable.size() || tablesize+1 == m_smalltable.size());
			if(tablesize+1 == m_smalltable.size()) { //! not found -> add a new node
				++m_elements;
				it = newleaf_id;
				DCHECK_EQ(it.value(), newleaf_id);
				return node_t(size() - 1, true); 
			}
			return node_t(it.value(), false); //! return the factor id of that node
		}

		// DCHECK_LT(large_key_width, m_rowlength);
		// if(m_rows[large_key_width] == nullptr) {
		// 	m_rows[large_key_width] = new table_t(large_key_width, value_max_width);
		// }
		// auto& table = *m_rows[large_key_width];
		// const size_t tablesize = table.size();
        // auto it = table[large_key];
		// DCHECK(tablesize == table.size() || tablesize+1 == table.size());

		// //TODO: shift large_key_width by m_smalltable_key_width
		
		if(large_key_width < m_smalltable_key_width + m_rowlength) {
			const size_t rowindex = large_key_width - m_smalltable_key_width; //! index in m_rows for this key
			DCHECK_LE(rowindex, m_rowlength); 
			if(m_rows[rowindex] == nullptr) {
				m_rows[rowindex] = new table_t(large_key_width-1, value_max_width);
			}
			const uint64_t subtract_key = 1ULL<<(large_key_width-1); //(-1ULL>>(64-large_key_width+1)); 
			auto& table = *m_rows[rowindex];
			DCHECK_LE(subtract_key, large_key);
			DCHECK_LT(bits_for(large_key - subtract_key), large_key_width);
			const squeeze_node_t remaining_key = large_key - subtract_key;
			DCHECK_GE(table.key_width(), bits_for(remaining_key));

			const size_t tablesize = table.size();
			auto it = table[remaining_key];
			DCHECK(tablesize == table.size() || tablesize+1 == table.size());

			if(tablesize+1 == table.size()) { //! not found -> add a new node
				++m_elements;
				// DCHECK_LT(newleaf_id, key);
				it = newleaf_id;
				DCHECK_EQ(it.value(), newleaf_id);
				return node_t(size() - 1, true); 
			}

			return node_t(it.value(), false); //! return the factor id of that node
		}
		DCHECK_LE(m_smalltable_key_width+m_rowlength, large_key_width);
		const size_t rowindex = large_key_width - m_smalltable_key_width - m_rowlength; //! index in m_matrix for this key
		DCHECK_LT(rowindex, m_matrixlength);
		if(m_matrix[rowindex] == nullptr) {
			m_matrix[rowindex] = new table_t*[value_max_width];
			std::fill(m_matrix[rowindex], m_matrix[rowindex]+value_max_width, nullptr);
		}
		
		const uint64_t subtract_key = 1ULL<<(large_key_width-1); //(-1ULL>>(64-large_key_width+1)); 
		DCHECK_LE(subtract_key, large_key);
		DCHECK_LT(bits_for(large_key - subtract_key), large_key_width);
		const squeeze_node_t remaining_key = large_key - subtract_key;
		table_t**const rowtable = m_matrix[rowindex];

		for(size_t k = 0; k < value_max_width; ++k) {
			if(rowtable[k] != nullptr) {
				DCHECK_GE(k, rowindex+m_smalltable_key_width+m_rowlength- ALPHABET_BITS); //TODO: use for optimization
				auto& table = *rowtable[k];
				const auto ret = table.locate(remaining_key);
				if(ret.second != -1ULL) { //! found node
					return node_t(table.value_at(ret.first, ret.second), false); //! return the factor id of that node
				}
			}
		}
		const uint_fast8_t columnindex = bits_for(newleaf_id);
		DCHECK_GE(columnindex, rowindex+m_smalltable_key_width+m_rowlength - ALPHABET_BITS); //TODO: use for optimization
		if(rowtable[columnindex] == nullptr) {
			rowtable[columnindex] = new table_t(large_key_width-1, columnindex);
		}
		rowtable[columnindex]->find_or_insert(remaining_key, newleaf_id);
		++m_elements;
		return node_t(size() - 1, true); 
    }

    inline size_t size() const {
		DCHECK_LT(m_elements, std::numeric_limits<factorid_t>::max());
        return m_elements;
    }
};


}} //ns
