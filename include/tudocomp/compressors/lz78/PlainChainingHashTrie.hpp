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


class PlainChainingHashTrie : public Algorithm, public LZ78Trie<> {
	static_assert(std::numeric_limits<squeeze_node_t>::radix == 2, "needs radix to be 2");
	static_assert(std::numeric_limits<factorid_t>::radix == 2, "needs radix to be 2");

	typedef separate_chaining::group::group_chaining_table<> table_t;
    // typedef std::unordered_map<squeeze_node_t, factorid_t, _VignaHasher> table_t;

	table_t m_table;

	size_t m_elements = 0;

	static constexpr uint_fast8_t value_max_width = std::min<uint_fast8_t>( bits_for(std::numeric_limits<len_t>::max() * ALPHABET_BITS / std::numeric_limits<len_t>::digits)  ,  std::numeric_limits<factorid_t>::digits);

public:
    inline static Meta meta() {
        Meta m(lz78_trie_type(), "plainchain", "Hash Trie with separate chaining (plain)");
        return m;
    }

    inline PlainChainingHashTrie(Config&& cfg, const size_t n, const size_t& remaining_characters, factorid_t reserve = 0)
        : Algorithm(std::move(cfg))
        , LZ78Trie(n, remaining_characters)
		, m_table(10, value_max_width)
    {
        if(reserve > 0) { m_table.reserve(reserve); }
    }
    IF_STATS(
        MoveGuard m_guard;
        inline ~PlainChainingHashTrie() {
            if (m_guard) {
			StatPhase logPhase;
			m_table.print_stats(logPhase);
            }
        }
    )
    PlainChainingHashTrie(PlainChainingHashTrie&& other) = default;
    PlainChainingHashTrie& operator=(PlainChainingHashTrie&& other) = default;

    inline node_t add_rootnode(uliteral_t c) {
		++m_elements;
		m_table[create_node(0, c)] = size();
        return node_t(size() - 1, true);
    }

    inline node_t get_rootnode(uliteral_t c) const {
        return node_t(c, false);
    }

    inline void clear() {
        m_table.clear();
		m_elements = 0;
    }

    inline node_t find_or_insert(const node_t& parent_w, uliteral_t c) {
        auto parent = parent_w.id();
        const factorid_t newleaf_id = size(); //! if we add a new node, its index will be equal to the current size of the dictionary

		const size_t tablesize = m_table.size();
		const size_t key = create_node(parent+1,c);
		if(bits_for(key) > m_table.key_width()) {
			m_table.reserve(m_table.bucket_count(), std::max(m_table.key_width(),bits_for(key)));
			// table_t tmp = table_t(std::max(m_table.key_width(),bits_for(key)), std::numeric_limits<squeeze_node_t>::digits);
			// tmp.reserve(m_table.bucket_count());
			// for(auto i = m_table.begin(); i != m_table.end(); ++i) {
			// 	tmp[i.key()] = i.value();
			// }
			// DCHECK_EQ(m_table.size(), tmp.size());
			// m_table = std::move(tmp);
			// std::cout << "bits: " << ((size_t) m_table.key_width()) << std::endl;
		}
		DCHECK_LE(bits_for(key), m_table.key_width());
        auto it = m_table[key];

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
