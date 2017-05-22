#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/lz78/LZ78Trie.hpp>
#include <tudocomp/compressors/lz78/squeeze_node.hpp>
#include <tudocomp/util/Hash.hpp>
#include <tudocomp_stat/StatPhase.hpp>

#include <unordered_map>

namespace tdc {
namespace lz78 {

class ExtHashTrie : public Algorithm, public LZ78Trie<factorid_t> {
	typedef std::unordered_map<squeeze_node_t, factorid_t, _VignaHasher> table_t;
//	typedef rigtorp::HashMap<squeeze_node_t, factorid_t,CLhash> table_t;
//	typedef ska::flat_hash_map<squeeze_node_t, factorid_t,CLhash> table_t; //ska::power_of_two_std_hash<size_t>> table_t;
//	typedef ska::flat_hash_map<squeeze_node_t, factorid_t,ska::power_of_two_std_hash<size_t>> table_t;
//	typedef rigtorp::HashMap<squeeze_node_t, factorid_t,_VignaHasher> table_t;

	table_t m_table;
  const size_t m_n;
  const size_t& m_remaining_characters;

public:
    inline static Meta meta() {
        Meta m("lz78trie", "exthash", "Hash Trie with external hash table");
		return m;
	}

    ExtHashTrie(Env&& env, const size_t n, const size_t& remaining_characters, factorid_t reserve = 0) 
		: Algorithm(std::move(env))
		, LZ78Trie(n, remaining_characters)
		, m_n(n)
		, m_remaining_characters(remaining_characters)
	{
	//	m_table.max_load_factor(0.9f);
		if(reserve > 0) {
			m_table.reserve(reserve);
		}
    }
    // ExtHashTrie(Env&& env, factorid_t reserve = 0) : Algorithm(std::move(env)) {
	// 	if(reserve > 0) {
	// 		m_table.reserve(reserve);
	// 	}
    // }
	IF_STATS(
		~ExtHashTrie() {
		StatPhase::log("table size", m_table.bucket_count());
		StatPhase::log("load factor", m_table.max_load_factor());
		StatPhase::log("entries", m_table.size());
		StatPhase::log("load ratio", m_table.load_factor());
		}
	);

	node_t add_rootnode(uliteral_t c) override {
		m_table.insert(std::make_pair<squeeze_node_t,factorid_t>(create_node(0, c), size()));
		return size() - 1;
	}

    node_t get_rootnode(uliteral_t c) override {
        return c;
    }

	void clear() override {
		m_table.clear();

	}

    node_t find_or_insert(const node_t& parent_w, uliteral_t c) override {
        auto parent = parent_w.id();
        const factorid_t newleaf_id = size(); //! if we add a new node, its index will be equal to the current size of the dictionary

		auto ret = m_table.insert(std::make_pair(create_node(parent,c), newleaf_id));


		if(ret.second) { // added a new node
			if(tdc_unlikely(m_table.bucket_count()*m_table.max_load_factor() < m_table.size()+1)) {
				const size_t expected_size = (m_table.size() + 1 + lz78_expected_number_of_remaining_elements(m_table.size(), m_n, m_remaining_characters))/0.95;
				if(expected_size < m_table.bucket_count()*2.0*0.95) {
					m_table.reserve(expected_size);
				}
			}
			return undef_id; // added a new node
		}


		if(ret.second) return undef_id; // added a new node
		return ret.first->second; // return the factor id of that node
    }

    factorid_t size() const override {
        return m_table.size();
    }
};

}} //ns

