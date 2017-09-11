#pragma once

#include <sdsl/suffix_trees.hpp>
#include <glog/logging.h>

#include <sstream>

using namespace sdsl;

//template<class bp_support = sdsl::bp_support_sada<> >
namespace tdc {
namespace lz78u {

/**
 * This is a wrapper class around the sdsl-lite library to get a easier translation between
 * the pseudocode in the LZCICS-paper and the C++ code
 */
struct SuffixTree {
    using cst_t = cst_sada<>;
	using node_type = cst_t::node_type;

	const cst_t& cst; //! sdsl suffix tree
	const cst_t::node_type root; //! the root node of the suffix tree
	const rank_support_v5<1> m_bp_rank1; //! rank the one bits in the BP
	const size_t internal_nodes; //! number of internal nodes

	SuffixTree(const cst_t& _cst)
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
		VLOG(2) << "LevelAnc of node " << node << " on depth " << depth << " is " << cst.bp_support.level_anc(node, cst.node_depth(node)-depth) << std::endl;
		return cst.bp_support.level_anc(node, cst.node_depth(node)-depth);
	}


	/**
	 * Select the i-th leaf in SA-order
	 * 0 <= i < n
	 */
	cst_t::node_type select_leaf(const cst_t::size_type& i) const {
		return cst.select_leaf(i+1);
	}


	/**
	 * Return the leaf corresponding to the suffix starting at position 0, i.e., the leaf with the longest string-depth
	 */
	cst_t::node_type smallest_leaf()const {
		return cst.select_leaf(cst.csa.isa[0]+1);
	}
	/*
	 * Given a leaf node, we select the leaf whose label is the label of node plus one.
	 */
	cst_t::node_type next_leaf(const cst_t::node_type& node)const {
		VLOG(2) << "Next Leaf " << node << "-> " << cst.select_leaf(cst.csa.psi[leafrank(node)]+1) << std::endl;
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
		VLOG(2) << "Next " << m << "-th Leaf " << node << "-> " << cst.select_leaf(m+1) << std::endl;
		return cst.select_leaf(val+1);
	}
	/*
	 * returns the number of preceding leaves of node
	 */
	cst_t::size_type leafrank(const cst_t::node_type& node)const{
		VLOG(2) << "Leafrank " << node << "-> " << cst.bp_rank_10.rank(node) << std::endl;
		return cst.bp_rank_10.rank(node);
	}

	/*
	 * Returns the length of the label read from the edges on the path from the root to node
	 */
	cst_t::size_type str_depth(const cst_t::node_type& node)const{
		return cst.depth(node);
	}

	/*
	 * Returns an unique ID for an internal node of the suffix tree
	 */
	cst_t::size_type nid(const cst_t::node_type& node)const{
		assert(!cst.is_leaf(node));
		VLOG(2) << "NID of node " << node << " is " << m_bp_rank1.rank(node) - cst.bp_rank_10.rank(node) << std::endl;
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

	class root_childrank_support {
		typedef sdsl::bp_support_sada<> bp_support;
		using cst_t = cst_sada<>;
		const bp_support& m_bp;

		sdsl::rrr_vector<> m_rc_rrrv;
		sdsl::rrr_vector<>::rank_1_type m_rc_rank;
		typedef bp_support::size_type size_type;

		//exported from SDSL
        inline static bool is_root(size_type v)
        {
            return v==0;
        }

		public:
		root_childrank_support(const bp_support& bp) : m_bp(bp) {
			const size_t m_size = bp.size();
			sdsl::bit_vector root_children(m_size);
			size_type rc = 1;

			while(rc < m_size - 1) {
				root_children[rc] = 1;
				rc = bp.find_close(rc) + 1;
			}
			m_rc_rrrv = sdsl::rrr_vector<>(root_children);
			m_rc_rank = sdsl::rrr_vector<>::rank_1_type(&m_rc_rrrv);
		}

	/**
	 * Returns how many preceding silbings node has.
     * node must be a direct child of the root.
     *  Child rank of node i (which MUST be a child of the root)
	 */
        size_type rank(size_type i) const {
            DCHECK(!is_root(i));
            DCHECK(is_root(m_bp.level_anc(i, 1))); //parent must be root!

            return m_rc_rank(i);
        }

		/**
		 * Returns the first character of the string on the path from the root to the node
		 */
	char head(const SuffixTree& st, const cst_t::node_type& node)const {
		//TODO: Test if this assert works!
		VLOG(2) << "child: " << rank(st.level_anc(node,1)) << std::endl;
		VLOG(2) << "Edge: " << (int) st.cst.csa.comp2char[rank(st.level_anc(node,1))]  << std::endl;
		//assert( cst.edge(level_anc(node,1), 1) ==
		//		cst.csa.comp2char[root_child_rank(level_anc(node,1))] );
		return st.cst.csa.comp2char[rank(st.level_anc(node,1))];
	}

	/*
	 * Returns the length of the label read from the edges on the path from the root to node
	 */
	cst_t::size_type str_depth(const SuffixTree& ST, const cst_t::node_type& node)const{
		if(ST.cst.is_leaf(node)) {
			return (ST.cst.size() <= 1) ? 0 : (ST.cst.size()-ST.cst.sn(node));
		}
		auto la = ST.cst.leftmost_leaf(ST.cst.select_child(node, 1));
		auto lb = ST.cst.leftmost_leaf(ST.cst.select_child(node, 2));
		size_t m = 0;
		while(head(ST,la) == head(ST,lb)) {
			la = ST.next_leaf(la);
			lb = ST.next_leaf(lb);
			++m;
		}
		DCHECK_EQ(m, ST.cst.depth(node));
		VLOG(2) << "Depth of " << node  << " is " << m << std::endl;
		return m;
	}
	};

}}//ns
