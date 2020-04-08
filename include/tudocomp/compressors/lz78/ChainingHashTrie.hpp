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


/**
 * @targ t_rowlength: gives the length of the array of hash tables in which a hash table stores a key with a specific key width
 * setting this number to something big (like 64) disables the matrix of hash tables.
 */
	template<uint_fast8_t t_rowlength = 10>
class ChainingHashTrie : public Algorithm, public LZ78Trie<> {
	static_assert(std::numeric_limits<squeeze_node_t>::radix == 2, "needs radix to be 2");
	static_assert(std::numeric_limits<factorid_t>::radix == 2, "needs radix to be 2");

	typedef separate_chaining::group::group_chaining_table<> table_t;
    // typedef std::unordered_map<squeeze_node_t, factorid_t, _VignaHasher> table_t;

	
	static constexpr uint_fast8_t value_max_width = std::min<uint_fast8_t>( bits_for(std::numeric_limits<len_t>::max() * ALPHABET_BITS / std::numeric_limits<len_t>::digits)  ,  std::numeric_limits<factorid_t>::digits);
	static constexpr uint_fast8_t key_max_width = std::min<uint_fast8_t>(value_max_width+ALPHABET_BITS, std::numeric_limits<squeeze_node_t>::digits);

	factorid_t m_max_rootnode = 0; //! the highest node ID of a node added with `add_rootnode`

	//! TODO: m_array is useless for LZW -> can we templatize this approach such that LZW does not use `m_array` at all?
	static constexpr size_t m_arraysize = 256;
	factorid_t m_array[m_arraysize];
	size_t m_elements = 0; //! number of stored elementsn including `m_max_rootnode`

	static constexpr size_t m_smalltable_key_width = 9;
	table_t m_smalltable;

	static constexpr uint_fast8_t m_rowlength = std::max<uint_fast8_t>(0, std::min<int32_t>(t_rowlength, static_cast<int32_t>(key_max_width) - m_smalltable_key_width));
	table_t* m_rows[m_rowlength];


	static constexpr uint_fast8_t m_matrixlength = 1 + std::max<int32_t>(0, static_cast<int32_t>(key_max_width) - m_rowlength - m_smalltable_key_width);
	table_t** m_matrix[m_matrixlength];
	uint8_t m_matrix_allocated[m_matrixlength]; //! stores the largest index of an allocated table in a row of m_matrix

	constexpr size_t matrixrow_offset(const size_t rowindex)  {
		return rowindex+m_smalltable_key_width+m_rowlength- ALPHABET_BITS;
	}

	constexpr size_t matrixrow_width(const size_t rowindex)  {
		return 1 + value_max_width - matrixrow_offset(rowindex);
	}

public:
    inline static Meta meta() {
        Meta m(lz78_trie_type(), std::string("chain") + std::to_string(t_rowlength), "Hash Trie with separate chaining");
        return m;
    }

    inline ChainingHashTrie(Config&& cfg, const size_t n, const size_t& remaining_characters, [[maybe_unused]] factorid_t reserve = 0)
        : Algorithm(std::move(cfg))
        , LZ78Trie(n, remaining_characters)
		, m_smalltable(m_smalltable_key_width, value_max_width)
    {
        // if(reserve > 0) { m_table.reserve(reserve); }
		std::fill(m_array, m_array+m_arraysize, -1ULL);
		if(m_rowlength > 0) {
		std::fill(m_rows, m_rows+m_rowlength, nullptr);
		}
		if(m_matrixlength > 0) {
			std::fill(m_matrix, m_matrix+m_matrixlength, nullptr);
			std::fill(m_matrix_allocated, m_matrix_allocated+m_matrixlength, 0);
		}
    }
    IF_STATS(
        MoveGuard m_guard;
    )
		inline ~ChainingHashTrie() {
    IF_STATS(
			StatPhase::wrap("chain stats", [&]{
            if (m_guard) {
			StatPhase::log("value_max_width", value_max_width);
			StatPhase::log("key_max_width", key_max_width);
			StatPhase::log("m_rowlength", m_rowlength);
			StatPhase::log("m_matrixlength", m_matrixlength);

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
					for(size_t k = 0; k < matrixrow_width(i); ++k) {
						if(m_matrix[i][k] == nullptr) { continue; }
						std::stringstream s;
						s << "m[" << std::setw(2) << i << "][" << std::setw(2) << (k+matrixrow_offset(i)) << "]";
						std::string strkey = s.str();
						StatPhase::log(std::move(strkey), m_matrix[i][k]->size());
					}
				}
			// StatPhase logPhase;
			// m_table.print_stats(logPhase);
            }//guard
			});//statphase
    )//STATS
			clear();
		}
    ChainingHashTrie(ChainingHashTrie&& other) = default;
    ChainingHashTrie& operator=(ChainingHashTrie&& other) = default;

    inline node_t add_rootnode(uliteral_t c) { //TODO: optimize -> seems that we do not need to store anything in m_array!
		m_max_rootnode = c;
		// DCHECK_LT(create_node(0, c), m_arraysize);
		// m_array[create_node(0, c)] = size();
		++m_elements;
		// m_table[create_node(0, c)] = size();
        return get_rootnode(c); 
    }

    inline node_t get_rootnode(uliteral_t c) const {
        return node_t(c, false);
    }

    inline void clear() {
		for(size_t i = 0; i < m_matrixlength; ++i) { 
			if(m_matrix[i] == nullptr) { continue; }
			for(size_t j = 0; j < matrixrow_width(i); ++j) {
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

		if(m_rowlength > 0) {
		std::fill(m_rows, m_rows+m_rowlength, nullptr);
		}
		if(m_matrixlength > 0) {
			std::fill(m_matrix, m_matrix+m_matrixlength, nullptr);
			std::fill(m_matrix_allocated, m_matrix_allocated+m_matrixlength, 0);
		}
		std::fill(m_array, m_array+m_arraysize, -1ULL);
		m_elements = 0;
		m_smalltable.clear();
    }

    inline node_t find_or_insert(const node_t& parent_w, uliteral_t c) {
        auto parent = parent_w.id();
        const factorid_t newleaf_id = size(); //! if we add a new node, its index will be equal to the current size of the dictionary

		squeeze_node_t key = create_node(parent,c);
		// DCHECK_NE(key, 0);
		// if(key <= m_max_rootnode) {
		// 	return node_t(key, false); //! return the factor id of that node
		// }
		// key -= m_max_rootnode;

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
			m_matrix[rowindex] = new table_t*[matrixrow_width(rowindex)];
			std::fill(m_matrix[rowindex], m_matrix[rowindex]+matrixrow_width(rowindex), nullptr);
		}
		
		const uint64_t subtract_key = 1ULL<<(large_key_width-1); //(-1ULL>>(64-large_key_width+1)); 
		DCHECK_LE(subtract_key, large_key);
		DCHECK_LT(bits_for(large_key - subtract_key), large_key_width);
		const squeeze_node_t remaining_key = large_key - subtract_key;
		table_t**const rowtable = m_matrix[rowindex];

		DCHECK_LT(m_matrix_allocated[rowindex], matrixrow_width(rowindex));
		ON_DEBUG(
			for(size_t l = m_matrix_allocated[rowindex]+1; l < matrixrow_width(rowindex); ++l) {
				DCHECK(rowtable[l] == nullptr);
			} 
		)
		for(size_t k = 0; k <= m_matrix_allocated[rowindex]; ++k) { //! check whether the key is in one of the hash tables of `rowtable`
			if(rowtable[k] != nullptr) {
				auto& table = *rowtable[k];
				const auto ret = table.locate(remaining_key);
				if(ret.second != -1ULL) { //! found node
					const uint_fast8_t value_width = k + matrixrow_offset(rowindex); //! have to restore the value by adding the subtracted amount when inserting it
					const factorid_t remaining_value = table.value_at(ret.first, ret.second);
					const factorid_t value = remaining_value + (1ULL<<(value_width-1));

					return node_t(value, false); //! return the factor id of that node
				}
			}

			//TODO: if rowtable == nullptr => all other are also nullptr? then `m_matrix_allocated` is not needed.
		}
		DCHECK_LE(matrixrow_offset(rowindex), bits_for(newleaf_id));
		const uint_fast8_t value_width = bits_for(newleaf_id);
		const uint_fast8_t columnindex = value_width - matrixrow_offset(rowindex);
		DCHECK_LT(columnindex, matrixrow_width(rowindex));
		if(rowtable[columnindex] == nullptr) {
			rowtable[columnindex] = new table_t(large_key_width-1, value_width-1); //! we save one value-bit by subtracting `subtract_value` in the following
			m_matrix_allocated[rowindex] = std::max<uint8_t>(m_matrix_allocated[rowindex], columnindex);
		}
		const uint64_t subtract_value = 1ULL<<(value_width-1); //(-1ULL>>(64-large_value_width+1)); 
		DCHECK_LE(subtract_value, newleaf_id);
		DCHECK_LT(bits_for(newleaf_id - subtract_value), value_width);
		const factorid_t remaining_value = newleaf_id - subtract_value;

		rowtable[columnindex]->find_or_insert(remaining_key, remaining_value);
		++m_elements;
		return node_t(size() - 1, true); 
    }

    inline size_t size() const {
		DCHECK_LT(m_elements, std::numeric_limits<factorid_t>::max());
        return m_elements;
    }
};

using ChainingHashTrie10 = ChainingHashTrie<10>;
using ChainingHashTrie0 = ChainingHashTrie<0>;
using ChainingHashTrie64 = ChainingHashTrie<20>;

}} //ns
