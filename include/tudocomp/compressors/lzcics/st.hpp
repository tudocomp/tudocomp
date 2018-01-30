#pragma once

#include <tudocomp/compressors/lzcics/cst_sada_light.hpp>
#include <tudocomp/compressors/lzcics/bp_support_sada.hpp>

#include <sdsl/suffix_trees.hpp>

namespace tdc {
namespace lzcics {

using namespace sdsl;

typedef cst_sada_light<csa_sada_light<>, bp_support_sadaM<>> cst_t;

/**
 * This is a wrapper class around the sdsl-lite library to get a easier translation between
 * the pseudocode in the LZCICS-paper and the C++ code
 */
struct ST {
	const cst_t& cst; //! sdsl suffix tree
	const cst_t::node_type root; //! the root node of the suffix tree
	const cst_t::size_type alpha; //! the \alpha-th leaf in the suffix tree has label 1
	const rank_support_v5<1> m_bp_rank1; //! rank the one bits in the BP
	const size_t internal_nodes; //! number of internal nodes
	
	ST(const cst_t& _cst) 
		: cst(_cst)
		, root(cst.root())
		, alpha(cst.csa.alpha())
		, m_bp_rank1(&cst.bp)
		, internal_nodes(m_bp_rank1.rank(m_bp_rank1.size()) - cst.bp_rank_10.rank(m_bp_rank1.size()))
	{
	}

	cst_t::node_type parent(const cst_t::node_type& node)const {
		return cst.parent(node);
	}
	/**
	 * Returns the level ancestor of node. 
	 * In the sdsl-implementation, the 0-th ancestor is the node itself.
	 * Here, we want that root is the 0-th ancestor.
	 */
	cst_t::node_type level_anc(const cst_t::node_type& node, size_t depth)const {
		DVLOG(2) << "LevelAnc of node " << node << " on depth " << depth << " is " << cst.bp_support.level_anc(node, cst.node_depth(node)-depth);
		return cst.bp_support.level_anc(node, cst.node_depth(node)-depth);
	}
	/**
	 * Returns how many preceding silbings node has.
     * node must be a direct child of the root.
	 */
    cst_t::size_type root_child_rank(const cst_t::node_type& node) const {
        return cst.bp_support.root_child_rank(node);
    }
	// cst_t::node_type leaf(const cst_t::node_type& node, const cst_t::size_type number)const {
	// 	return cst.select_leaf(number+1+cst.lb(node));
	// }
	// cst_t::node_type child(const cst_t::node_type& node, const cst_t::size_type number)const {
	// 	return cst.select_child(node, number);
	// }
	char head(const cst_t::node_type& node)const { 
		//TODO: Test if this assert works!
		DVLOG(2) << "child: " << root_child_rank(level_anc(node,1));
		DVLOG(2) << "Edge: " << (int) cst.csa.comp2char[root_child_rank(level_anc(node,1))];
		//assert( cst.edge(level_anc(node,1), 1) == 
		//		cst.csa.comp2char[root_child_rank(level_anc(node,1))] );
		return cst.csa.comp2char[root_child_rank(level_anc(node,1))]; 
	}
	/**
	 * Return the leaf corresponding to the suffix starting at position 0, i.e., the leaf with the longest string-depth
	 */
	cst_t::node_type smallest_leaf()const {
		return cst.select_leaf(alpha+1);
	}
	/*
	 * Given a leaf node, we select the leaf whose label is the label of node plus one.
	 */
	cst_t::node_type next_leaf(const cst_t::node_type& node)const {
		DVLOG(2) << "Next Leaf " << node << "-> " << cst.select_leaf(cst.csa.psi[leafrank(node)]+1);
		return cst.select_leaf(cst.csa.psi[leafrank(node)]+1);
	}
	/*
	 * apply m times next-leaf
	 */
	cst_t::node_type next_mth_leaf(const cst_t::node_type& node, cst_t::size_type m)const {
		cst_t::size_type val = leafrank(node);
		while(m > 0) {
			val = cst.csa.psi[val];
			--m;
		}
		DVLOG(2) << "Next " << m << "-th Leaf " << node << "-> " << cst.select_leaf(m+1);
		return cst.select_leaf(val+1);
	}
	/*
	 * returns the number of preceding leaves of node
	 */
	cst_t::size_type leafrank(const cst_t::node_type& node)const{
		DVLOG(2) << "Leafrank " << node << "-> " << cst.bp_rank_10.rank(node);
		return cst.bp_rank_10.rank(node);
	}
	/*
	 * Returns the length of the label read from the edges on the path from the root to node
	 */
	cst_t::size_type str_depth(const cst_t::node_type& node)const{
		DCHECK(!cst.is_leaf(node)); // we do not support leaves
		auto la = cst.leftmost_leaf(cst.select_child(node, 1));
		auto lb = cst.leftmost_leaf(cst.select_child(node, 2));
		size_t m = 0;
		while(head(la) == head(lb)) {
			la = next_leaf(la);
			lb = next_leaf(lb);
			++m;
		}
		//assert(m == cst.depth(node));
		DVLOG(2) << "Depth of " << node  << " is " << m;
		return m;
	}
	/*
	 * Returns an unique ID for an internal node of the suffix tree
	 */
	cst_t::size_type nid(const cst_t::node_type& node)const{
		assert(!cst.is_leaf(node));
		DVLOG(2) << "NID of node " << node << " is " << m_bp_rank1.rank(node) - cst.bp_rank_10.rank(node);
		return m_bp_rank1.rank(node)- cst.bp_rank_10.rank(node);
	}

	/**
	 * Returns the number of leaves contained in the subtree rooted at 'node'
	 */
	cst_t::size_type number_of_leaves(const cst_t::node_type& node)const{ // only for DEBUG
		return cst.size(node);
	}

};

void reset_bitvector(bit_vector& bv) { //! resets a bit-vector, clearing all ones
	for(auto it = bv.begin(); it != bv.end(); ++it) *it = 0; //TODO: is this the fastest approach?
}

//get suffix tree for text
ST suffix_tree(const uliteral_t* text, cst_t& cst) {
    construct_im(cst, (const char*)text, 1); //FIXME: SDSL s*cks
    return ST(cst);
}

}} //ns

