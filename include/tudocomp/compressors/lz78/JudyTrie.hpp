#ifndef JUDYTRIE_HPP
#define JUDYTRIE_HPP
#include <tudocomp/config.h>
#ifdef JUDY_H_AVAILABLE

/**
 * LZ78 Trie Implementation 
 * based on Julius Pettersson (MIT/Expat License.) and Juha Nieminen's work.
 * @see http://www.cplusplus.com/articles/iL18T05o/
**/

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/lz78/LZ78Trie.hpp>
#include <tudocomp/compressors/lz78/squeeze_node.hpp>
#include <Judy.h> // Judy Array

namespace tdc {
namespace lz78 {


class JudyTrie : public Algorithm, public LZ78Trie<factorid_t> {

	Pvoid_t m_dict; // judy array
	size_t m_size;

	factorid_t& find(const node_t& node) {
		void* pvalue;
		JLI(pvalue, m_dict, node.id()); //lookup node in dict, store value in pvalue
		DCHECK_NE(pvalue, PJERR);
		return *reinterpret_cast<factorid_t*>(pvalue);
	}

public:
    inline static Meta meta() {
        Meta m("lz78trie", "judy", "Lempel-Ziv 78 Judy Array");
		return m;
	}
    JudyTrie(Env&& env, factorid_t = 0) 
		: Algorithm(std::move(env))
	    , m_dict(static_cast<Pvoid_t>(nullptr))	
	    , m_size(0)
	{
    }

    node_t get_rootnode(uliteral_t c) override {
        return create_node(0,c);
    }

	node_t add_rootnode(uliteral_t c) override {
		DCHECK_EQ(find(create_node(0, c)), 0);
		find(create_node(0, c)) = size();
		++m_size;
		return size();
	}

	void clear() override {
		if(m_dict != nullptr) {
			int i;
			JLFA(i, m_dict);
		}
		m_size = 0; 
	}

    node_t find_or_insert(const node_t& parent, uliteral_t c) override {
		const node_t node = create_node(parent.id(), c);

		factorid_t& id = find(node);

		if(id == 0) {
        	const factorid_t newleaf_id = size()+1; //! if we add a new node, its index will be equal to the current size of the dictionary
			id = newleaf_id;
			++m_size;
			return undef_id;
		}
		return id-1;
    }

    factorid_t size() const override {
        return m_size;
    }
};

}} //ns

#endif// JUDY_H_AVAILABLE
#endif /* JUDYTRIE_HPP */
