#pragma once

#include <unordered_map>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/lz78/LZ78common.hpp>
#include <tudocomp/compressors/lz78/squeeze_node.hpp>

namespace tdc {
namespace lz78 {

class HashTrie : public Algorithm, public LZ78Trie<factorid_t> {
    using squeze_node_t = ::tdc::lz78::node_t;
	std::unordered_map<squeze_node_t, factorid_t> table;

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

	node_t add_rootnode(uliteral_t c) override {
		table.insert(std::make_pair<squeze_node_t,factorid_t>(create_node(0, c), size()));
		return size() - 1;
	}

    node_t get_rootnode(uliteral_t c) override {
        return c;
    }

	void clear() override {
		table.clear();

	}

    node_t find_or_insert(const node_t& parent_w, uliteral_t c) override {
        auto parent = parent_w.id();
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

