/* sdsl - succinct data structures library
    Copyright (C) 2009-2013 Simon Gog

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see http://www.gnu.org/licenses/ .
*/
/*
    Modified "light" version with reduced feature support for higher
    memory and construction time efficiency.
*/
/*! \file cst_sada_light.hpp
    \brief cst_sada_light.hpp contains a lightweight implementation of
           Sadakane's CST.
    \author Simon Gog
*/
#pragma once

#include <sdsl/int_vector.hpp>
#include <sdsl/iterators.hpp>
#include <sdsl/select_support_mcl.hpp>
#include <sdsl/bp_support.hpp>
#include <sdsl/bp_support_sada.hpp>
#include <tudocomp/compressors/lzcics/csa_sada_light.hpp> // for std initialization of cst_sada
#include <sdsl/cst_iterators.hpp>
#include <sdsl/cst_sct3.hpp>
#include <sdsl/sdsl_concepts.hpp>
#include <sdsl/construct.hpp>
#include <sdsl/suffix_tree_helper.hpp>
#include <sdsl/suffix_tree_algorithm.hpp>
#include <sdsl/util.hpp>
#include <iostream>
#include <algorithm>
#include <cassert>
#include <cstring> // for strlen
#include <iomanip>
#include <iterator>

namespace tdc {
namespace lzcics {

using namespace sdsl;

//! A class for the Compressed Suffix Tree (CST) proposed by Sadakane.
/*!
 * \tparam t_csa       Type of a CSA (member of this type is accessible via
 *                     member `csa`, default class is sdsl::csa_sada).
 * \tparam t_bp_support Type of a BPS structure (member accessible via member
 *                      `bp_support`, default class is sdsl::bp_support_sada),
 * \tparam t_rank_10    Type of a rank structure for the 2-bit pattern `10`
 *                      (accessible via member `bp_rank_10`, default class is
 *                      sdsl::rank_support_v5)
 * \tparam t_select_10  Type of a select structure for the 2-bit pattern `10`
 *                      (accessible via member \f$bp\_select\_10\f$, default
 *                      class is sdsl::select_support_mcl).
 *
 * It also contains a sdsl::bit_vector which represents the balanced
 * parentheses sequence of the suffix tree. This bit_vector can be accessed
 * via member `bp`.
 *
 * A node `v` of the `csa_sada` is represented by an integer `i` which
 * corresponds to the position of the opening parenthesis of the parentheses
 * pair \f$(i,\mu(i))\f$ that corresponds to `v` in `bp`.
 *
 * \par Reference
 *  Kunihiko Sadakane:
 *  Compressed Suffix Trees with Full Functionality.
 *  Theory Comput. Syst. 41(4): 589-607 (2007)
 *
 * @ingroup cst
 */
template<class t_csa = csa_sada_light<>,
         class t_bp_support = bp_support_sada<>,
         class t_rank_10 = rank_support_v5<10,2>,
         class t_select_10 = select_support_mcl<10,2>
         >
class cst_sada_light
{
        static_assert(std::is_same<typename index_tag<t_csa>::type, csa_tag>::value,
                      "First template argument has to be a compressed suffix array.");
    public:
        typedef typename t_csa::size_type                         size_type;
        typedef t_csa                                             csa_type;
        typedef typename t_csa::char_type                         char_type;
        typedef size_type                                         node_type; //!< Type for the nodes  in the tree.
        typedef t_bp_support                                      bp_support_type;
        typedef t_rank_10                                         rank_10_type;
        typedef t_select_10                                       select_10_type;

        typedef typename t_csa::alphabet_type::comp_char_type     comp_char_type;
        typedef typename t_csa::alphabet_type::sigma_type         sigma_type;

        typedef typename t_csa::alphabet_category                 alphabet_category;
        typedef cst_tag                                           index_category;
    private:
        t_csa           m_csa; // suffix array
        bit_vector      m_bp;  // balanced parentheses sequence for suffix tree
        bp_support_type m_bp_support; // support for the balanced parentheses sequence
        rank_10_type    m_bp_rank10;  // rank_support for leaves, i.e. "10" bit pattern
        select_10_type  m_bp_select10;// select_support for leaves, i.e. "10" bit pattern

        /* Get the number of leaves that are in the subtree rooted at the first child of v +
         * number of leafs in the subtrees rooted at the children of parent(v) which precede v in the tree.
         */
        size_type inorder(node_type v)const
        {
            return m_bp_rank10(m_bp_support.find_close(v+1)+1);
        }

        void copy(const cst_sada_light& cst)
        {
            m_csa           = cst.m_csa;
            m_bp            = cst.m_bp;
            m_bp_support    = cst.m_bp_support;
            m_bp_support.set_vector(&m_bp);
            m_bp_rank10     = cst.m_bp_rank10;
            m_bp_rank10.set_vector(&m_bp);
            m_bp_select10   = cst.m_bp_select10;
            m_bp_select10.set_vector(&m_bp);
        }

    public:
        const t_csa&           csa          = m_csa;
        const bit_vector&      bp           = m_bp;
        const bp_support_type& bp_support   = m_bp_support;
        const rank_10_type&    bp_rank_10   = m_bp_rank10;
        const select_10_type&  bp_select_10 = m_bp_select10;

//! Default constructor
        cst_sada_light() { }


//! Copy constructor
        cst_sada_light(const cst_sada_light& cst)
        {
            copy(cst);
        }

//! Move constructor
        cst_sada_light(cst_sada_light&& cst)
        {
            *this = std::move(cst);
        }

//! Construct CST from file_map
        cst_sada_light(cache_config& config)
        {
            {
                auto event = memory_monitor::event("bps-dfs");
                sdsl::cst_sct3<> temp_cst(config, true);
                m_bp.resize(4*(temp_cst.bp.size()/2));
                util::set_to_value(m_bp, 0);
                size_type idx=0;
                for (auto it=temp_cst.begin(), end=temp_cst.end(); it!=end; ++it) {
                    if (1 == it.visit())
                        m_bp[idx] = 1;
                    if (temp_cst.is_leaf(*it) and temp_cst.root()!= *it)
                        ++idx;
                    ++idx;
                }
                m_bp.resize(idx);
            }
            {
                auto event = memory_monitor::event("bpss-dfs");
                util::assign(m_bp_support, bp_support_type(&m_bp));
                util::init_support(m_bp_rank10,   &m_bp);
                util::init_support(m_bp_select10, &m_bp);
            }
            {
                auto event = memory_monitor::event("load csa");
                load_from_cache(m_csa,std::string(conf::KEY_CSA)+"_"+util::class_to_hash(m_csa), config);
            }
        }

//! Number of leaves in the suffix tree.
        /*! Required for the Container Concept of the STL.
         *  \sa max_size, empty
         */
        size_type size()const
        {
            return m_csa.size();
        }

//! Returns the maximal lenght of text for that a suffix tree can be build.
        /*! Required for the Container Concept of the STL.
         *  \sa size
         */
        static size_type max_size()
        {
            return t_csa::max_size();
        }

//! Returns if the data strucutre is empty.
        /*! Required for the Container Concept of the STL.
         * \sa size
         */
        bool empty()const
        {
            return m_csa.empty();
        }

//! Swap method for cst_sada_light
        /*! The swap method can be defined in terms of assignment.
            This requires three assignments, each of which, for a container type, is linear
            in the container's size. In a sense, then, a.swap(b) is redundant.
            This implementation guaranties a run-time complexity that is constant rather than linear.
            \param cst cst_sada_light to swap.

            Required for the Assignable Conecpt of the STL.
          */
        void swap(cst_sada_light& cst)
        {
            if (this != &cst) {
                m_csa.swap(cst.m_csa);
                m_bp.swap(cst.m_bp);
                util::swap_support(m_bp_support, cst.m_bp_support, &m_bp, &(cst.m_bp));
                util::swap_support(m_bp_rank10, cst.m_bp_rank10, &m_bp, &(cst.m_bp));
                util::swap_support(m_bp_select10, cst.m_bp_select10, &m_bp, &(cst.m_bp));
            }
        }

//! Assignment Operator.
        /*!
         *    Required for the Assignable Concept of the STL.
         */
        cst_sada_light& operator=(const cst_sada_light& cst)
        {
            if (this != &cst) {
                copy(cst);
            }
            return *this;
        }

//! Assignment Move Operator.
        /*!
         *    Required for the Assignable Concept of the STL.
         */
        cst_sada_light& operator=(cst_sada_light&& cst)
        {
            if (this != &cst) {
                m_csa           = std::move(cst.m_csa);
                m_bp            = std::move(cst.m_bp);
                m_bp_support    = std::move(cst.m_bp_support);
                m_bp_support.set_vector(&m_bp);
                m_bp_rank10     = std::move(cst.m_bp_rank10);
                m_bp_rank10.set_vector(&m_bp);
                m_bp_select10   = std::move(cst.m_bp_select10);
                m_bp_select10.set_vector(&m_bp);
            }
            return *this;
        }

//! Serialize to a stream.
        /*! \param out Outstream to write the data structure.
         *  \return The number of written bytes.
         */
        size_type serialize(std::ostream& out, structure_tree_node* v=nullptr, std::string name="")const
        {
            structure_tree_node* child = structure_tree::add_child(v, name, util::class_name(*this));
            size_type written_bytes = 0;
            written_bytes += m_csa.serialize(out, child, "csa");
            written_bytes += m_bp.serialize(out, child, "bp");
            written_bytes += m_bp_support.serialize(out, child, "bp_support");
            written_bytes += m_bp_rank10.serialize(out, child, "bp_rank_10");
            written_bytes += m_bp_select10.serialize(out, child, "bp_select_10");
            structure_tree::add_size(child, written_bytes);
            return written_bytes;
        }

//! Load from a stream.
        /*! \param in Inputstream to load the data structure from.
         */
        void load(std::istream& in)
        {
            m_csa.load(in);
            m_bp.load(in);
            m_bp_support.load(in, &m_bp);
            m_bp_rank10.load(in, &m_bp);
            m_bp_select10.load(in, &m_bp);
        }

        /*! \defgroup cst_sada_tree_methods Tree methods of cst_sada */
        /* @{ */

//! Return the root of the suffix tree.
        /*!
         * \par Time complexity
         *   \f$ \Order{1} \f$
         */
        node_type root() const
        {
            return 0;
        }

//! Decide if a node is a leaf in the suffix tree.
        /*!
        * \param v A valid node of a cst_sada.
        * \returns A boolean value indicating if v is a leaf.
        * \par Time complexity
        *      \f$ \Order{1} \f$
        */
        bool is_leaf(node_type v)const
        {
            assert(m_bp[v]==1);  // assert that v is a valid node of the suffix tree
            // if there is a closing parenthesis at position v+1, the node is a leaf
            return !m_bp[v+1];
        }

//! Return the i-th leaf (1-based from left to right) of the suffix tree.
        /*!
         * \param i 1-based position of the leaf. \f$1\leq i\leq csa.size()\f$.
         * \return The i-th leave.
         * \par Time complexity
         *     \f$ \Order{1} \f$
         * \pre \f$ 1 \leq i \leq csa.size() \f$
         */
        node_type select_leaf(size_type i)const
        {
            assert(i > 0 and i <= m_csa.size());
            // -1 as select(i) returns the postion of the 0 of pattern 10
            return m_bp_select10.select(i)-1;
        }

//! Returns the node depth of node v.
        /*!
         * \param v A valid node of a cst_sada.
         * \return The node depth of node v.
         * \par Time complexity
         *   \f$ \Order{1} \f$
         */
        size_type node_depth(node_type v)const
        {
            // -2 as the root() we assign depth=0 to the root
            return (m_bp_support.rank(v)<<1)-v-2;
        }

//! Calculate the number of leaves in the subtree rooted at node v.
        /*! \param v A valid node of the suffix tree.
         *  \return The number of leaves in the subtree rooted at node v.
         *  \par Time complexity
         *      \f$ \Order{1} \f$
         *
         *  This method is used e.g. in the count method.
         */
        size_type size(node_type v)const
        {
            size_type r = m_bp_support.find_close(v);
            return m_bp_rank10(r+1) - m_bp_rank10(v);
        }

//! Calculates the leftmost leaf in the subtree rooted at node v.
        /*! \param v A valid node of the suffix tree.
         *  \return The leftmost leaf in the subtree rooted at node v.
         *  \par Time complexity
         *    \f$ \Order{1} \f$
         */
        node_type leftmost_leaf(const node_type v)const
        {
            return m_bp_select10(m_bp_rank10(v)+1)-1;
        }

//! Calculate the parent node of a node v.
        /*! \param v A valid node of the suffix tree.
         *  \return The parent node of v or root() if v equals root().
         *  \par Time complexity
         *       \f$ \Order{1} \f$
         */
        node_type parent(node_type v) const
        {
            assert(m_bp[v]==1); // assert a valid node
            if (v == root())
                return root();
            else {
                return m_bp_support.enclose(v);
            }
        }

        //! Get the i-th child of a node v.
        /*!
         * \param v A valid tree node of the cst.
         * \param i 1-based Index of the child which should be returned. \f$i \geq 1\f$.
         * \return The i-th child node of v or root() if v has no i-th child.
         * \par Time complexity
         *   \f$ \Order{i} \f$ for \f$  i \leq \sigma \f$
         *  \pre \f$ 1 \leq i \leq degree(v) \f$
         */
        node_type select_child(node_type v, size_type i)const
        {
            if (is_leaf(v))  // if v is a leave, v has no child
                return root();
            size_type res = v+1;
            while (i > 1) {
                res = m_bp_support.find_close(res)+1;
                if (!m_bp[res]) {// closing parenthesis: there exists no next child
                    return root();
                }
                --i;
            }
            return res;
        }

//! Get the number of nodes of the suffix tree.
        /*
         *  \return The number of nodes of the suffix tree.
         *  \par Time complexity
         *    \f$ \Order{1} \f$
         */
        size_type nodes()const
        {
            return m_bp.size()>>1;
        }
        /* @} */
};

}} // end namespace

