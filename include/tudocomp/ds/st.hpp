#pragma once

#include "cst_sada_light.hpp"
#include <sdsl/suffix_trees.hpp>

#include "bp_support_sada.hpp"
#include <sstream>

using namespace sdsl;

namespace tdc {

/**
 * This is a wrapper class around the sdsl-lite library to get a easier translation between
 * the pseudocode in the LZCICS-paper and the C++ code
 */
struct STLz78u {
    using cst_t = cst_sada<>;
    using node_type = cst_t::node_type;

	const cst_t& cst; //! sdsl suffix tree
	const cst_t::node_type root; //! the root node of the suffix tree

	const rank_support_v5<1> m_bp_rank1; //! rank the one bits in the BP
	const size_t internal_nodes; //! number of internal nodes

	STLz78u(const cst_t& _cst)
		: cst(_cst)
		, root(cst.root())
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
		DLOG(INFO) << "LevelAnc of node " << node << " on depth " << depth << " is " << cst.bp_support.level_anc(node, cst.node_depth(node)-depth) << std::endl;
		return cst.bp_support.level_anc(node, cst.node_depth(node)-depth);
	}

	/*
	 * Given a leaf node, we select the leaf whose label is the label of node plus one.
	 */
	cst_t::node_type next_leaf(const cst_t::node_type& node)const {
		DLOG(INFO) << "Next Leaf " << node << "-> " << cst.select_leaf(cst.csa.psi[leafrank(node)]+1) << std::endl;
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
		DLOG(INFO) << "Next " << m << "-th Leaf " << node << "-> " << cst.select_leaf(m+1) << std::endl;
		return cst.select_leaf(val+1);
	}
	/*
	 * returns the number of preceding leaves of node
	 */
	cst_t::size_type leafrank(const cst_t::node_type& node)const{
		DLOG(INFO) << "Leafrank " << node << "-> " << cst.bp_rank_10.rank(node) << std::endl;
		return cst.bp_rank_10.rank(node);
	}
	/*
	 * Returns the length of the label read from the edges on the path from the root to node
	 */
	cst_t::size_type str_depth(const cst_t::node_type& node)const{
		auto m = cst.depth(node);
		DLOG(INFO) << "Depth of " << node  << " is " << m << std::endl;
		return m;
	}
	/*
	 * Returns an unique ID for an internal node of the suffix tree
	 */
	cst_t::size_type nid(const cst_t::node_type& node)const{
		assert(!cst.is_leaf(node));
		DLOG(INFO) << "NID of node " << node << " is " << m_bp_rank1.rank(node) - cst.bp_rank_10.rank(node) << std::endl;
		return m_bp_rank1.rank(node)- cst.bp_rank_10.rank(node);
	}

	/**
	 * Returns the number of leaves contained in the subtree rooted at 'node'
	 */
	cst_t::size_type number_of_leaves(const cst_t::node_type& node)const{ // only for DEBUG
		return cst.size(node);
	}

};

inline void reset_bitvector(bit_vector& bv) { //! resets a bit-vector, clearing all ones
	sdsl::util::set_to_value(bv, 0);
}

}
