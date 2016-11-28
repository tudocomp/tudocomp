#ifndef HASHTRIE_HPP
#define HASHTRIE_HPP

#include <unordered_map>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/lz78/LZ78common.hpp>
#include <tudocomp/compressors/lz78/squeeze_node.hpp>

namespace tdc {
namespace lz78 {

class HashTrie : public Algorithm, public LZ78Trie {

	std::unordered_map<node_t, factorid_t> table;

public:
    inline static Meta meta() {
        Meta m("lz78trie", "hash", "Lempel-Ziv 78 Hash Trie");
		return m;
	}

    HashTrie(Env&& env, factorid_t reserve = 0) : Algorithm(std::move(env)) {
		if(reserve > 0) {
			table.reserve(reserve);
		}
    }

	factorid_t add_rootnode(uliteral_t c) override {
		table.insert(std::make_pair<node_t,factorid_t>(create_node(0, c), size()));
		return size();
	}

	void clear() override {
		table.clear();

	}

    factorid_t find_or_insert(const factorid_t& parent, uliteral_t c) override {
        const factorid_t newleaf_id = size(); //! if we add a new node, its index will be equal to the current size of the dictionary

		auto ret = table.insert(std::make_pair(create_node(parent,c), newleaf_id));
		if(ret.second) return undef_id; // added a new node
		return ret.first->second; // return the factor id of that node
    }

    factorid_t size() const override {
        return table.size();
    }
};

}} //ns

#endif /* HASHTRIE_HPP */
