#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/util/Hash.hpp>
#include <tudocomp/compressors/lz78/LZ78Trie.hpp>
#include <tudocomp/compressors/lz78/squeeze_node.hpp>
#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {
namespace lz78 {


template<class HashFunction, class HashProber, class HashManager>
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

    HashTrie(Env&& env, const size_t n, const size_t& remaining_characters, factorid_t reserve = 0) 
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
		~HashTrie() {
			m_table.collect_stats(env());
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
//		table.clear();

	}

    node_t find_or_insert(const node_t& parent_w, uliteral_t c) override {
        auto parent = parent_w.id();
        const factorid_t newleaf_id = size(); //! if we add a new node, its index will be equal to the current size of the dictionary

		auto ret = m_table.insert(std::make_pair(create_node(parent,c), newleaf_id));
		if(ret.second) return undef_id; // added a new node
		return ret.first.value();
    }

    factorid_t size() const override {
        return m_table.entries();
    }
};

}} //ns

