/* sdsl - succinct data structures library
    Copyright (C) 2008-2013 Simon Gog

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
/*! \file csa_sada_light.hpp
    \brief csa_sada_light.hpp contains a lightweight implementation of
           the compressed suffix array.
    \author Simon Gog
*/
#ifndef INCLUDED_SDSL_CSA_SADA_LIGHT
#define INCLUDED_SDSL_CSA_SADA_LIGHT

#include <sdsl/enc_vector.hpp>
#include <sdsl/int_vector.hpp>
#include <sdsl/iterators.hpp>
#include <sdsl/suffix_array_helper.hpp>
#include <sdsl/util.hpp>
#include <sdsl/csa_sampling_strategy.hpp>
#include <sdsl/csa_alphabet_strategy.hpp>
#include <iostream>
#include <algorithm>
#include <cassert>
#include <cstring> // for strlen
#include <iomanip>
#include <iterator>

namespace sdsl
{

//! A class for the Compressed Suffix Array (CSA) proposed by Sadakane for practical implementation.
/*!
  *  \tparam t_enc_vec         Space-efficient vector for increasing integer sequences.
  *  \tparam t_dens            Sampling density of SA values
  *  \tparam t_int_dens        Sampling density of ISA values
  *  \tparam t_sa_sample_strat Policy of SA sampling. E.g. sample in SA-order or text-order.
  *  \tparam t_isa             Vector type for ISA sample values.
  *  \tparam t_alphabet_strat  Policy for alphabet representation.
  *
  *  \sa sdsl::csa_wt, sdsl::csa_bitcompressed
  * @ingroup csa
 */
template<class t_enc_vec         = enc_vector<>,          // Vector type used to store the Psi-function
         uint32_t t_dens         = 32,                    // Sample density for suffix array (SA) values
         uint32_t t_inv_dens     = 64,                    // Sample density for inverse suffix array (ISA) values
         class t_sa_sample_strat = sa_order_sa_sampling<>,// Policy class for the SA sampling.
         class t_isa_sample_strat= isa_sampling<>,        // Policy class for ISA sampling.
         class t_alphabet_strat  = byte_alphabet          // Policy class for the representation of the alphabet.
         >
class csa_sada_light
{
        static_assert(is_enc_vec<t_enc_vec>::value,
                      "First template argument has to be of type env_vector.");
        static_assert(t_dens > 0,
                      "Second template argument has to be greater then 0.");
        static_assert(t_inv_dens > 0,
                      "Third template argument has to be greater then 0.");
        static_assert(std::is_same<typename sampling_tag<t_sa_sample_strat>::type, sa_sampling_tag>::value,
                      "Forth template argument has to be a suffix array sampling strategy.");
        static_assert(std::is_same<typename sampling_tag<t_isa_sample_strat>::type, isa_sampling_tag>::value,
                      "Fifth template argument has to be a inverse suffix array sampling strategy.");
        static_assert(is_alphabet<t_alphabet_strat>::value,
                      "Sixth template argument has to be a alphabet strategy.");

        friend class bwt_of_csa_psi<csa_sada_light>;
    public:
        enum { sa_sample_dens = t_dens,
               isa_sample_dens = t_inv_dens
             };

        typedef uint64_t                                             value_type;
        typedef random_access_const_iterator<csa_sada_light>         const_iterator;
        typedef const_iterator                                       iterator;
        typedef const value_type                                     const_reference;
        typedef const_reference                                      reference;
        typedef const_reference*                                     pointer;
        typedef const pointer                                        const_pointer;
        typedef int_vector<>::size_type                              size_type;
        typedef size_type                                            csa_size_type;
        typedef ptrdiff_t                                            difference_type;
        typedef t_enc_vec                                            enc_vector_type;
        typedef enc_vector_type                                      psi_type;
        typedef traverse_csa_psi<csa_sada_light,false>               lf_type;
        typedef bwt_of_csa_psi<csa_sada_light>                       bwt_type;
        typedef isa_of_csa_psi<csa_sada_light>                       isa_type;
        typedef text_of_csa<csa_sada_light>                          text_type;
        typedef first_row_of_csa<csa_sada_light>                     first_row_type;
        typedef typename t_sa_sample_strat::template type<csa_sada_light>  sa_sample_type;
        typedef typename t_isa_sample_strat::template type<csa_sada_light> isa_sample_type;
        typedef t_alphabet_strat                                     alphabet_type;
        typedef typename alphabet_type::alphabet_category            alphabet_category;
        typedef typename alphabet_type::comp_char_type               comp_char_type;
        typedef typename alphabet_type::char_type                    char_type; // Note: This is the char type of the CSA not the WT!
        typedef typename alphabet_type::string_type                  string_type;
        typedef csa_sada_light                                       csa_type;

        typedef csa_tag                                              index_category;
        typedef psi_tag                                              extract_category;

        friend class traverse_csa_psi<csa_sada_light,true>;
        friend class traverse_csa_psi<csa_sada_light,false>;

        static const uint32_t linear_decode_limit = 100000;
    private:
        enc_vector_type m_psi;        // psi function
        size_type       m_alpha;      //! the \alpha-th leaf in the suffix tree has label 1
        alphabet_type   m_alphabet;   // alphabet component

        mutable std::vector<uint64_t> m_psi_buf; // buffer for decoded psi values

        void copy(const csa_sada_light& csa)
        {
            m_psi        = csa.m_psi;
            m_alpha      = csa.m_alpha;
            m_alphabet   = csa.m_alphabet;
        };

        void create_buffer()
        {
            if (enc_vector_type::sample_dens < linear_decode_limit) {
                m_psi_buf = std::vector<uint64_t>(enc_vector_type::sample_dens+1);
            }
        }

    public:
        const typename alphabet_type::char2comp_type& char2comp  = m_alphabet.char2comp;
        const typename alphabet_type::comp2char_type& comp2char  = m_alphabet.comp2char;
        const typename alphabet_type::C_type&         C          = m_alphabet.C;
        const typename alphabet_type::sigma_type&     sigma      = m_alphabet.sigma;
        const psi_type&                               psi        = m_psi;
        const lf_type                                 lf         = lf_type(*this);
        const bwt_type                                bwt        = bwt_type(*this);
        const bwt_type                                L          = bwt_type(*this);
        const first_row_type                          F          = first_row_type(*this);
        const text_type                               text       = text_type(*this);

        //! Default Constructor
        csa_sada_light()
        {
            create_buffer();
        }
        //! Default Destructor
        ~csa_sada_light() { }

        //! Copy constructor
        csa_sada_light(const csa_sada_light& csa)
        {
            create_buffer();
            copy(csa);
        }

        //! Move constructor
        csa_sada_light(csa_sada_light&& csa)
        {
            *this = std::move(csa);
        }

        csa_sada_light(cache_config& config);

        //! Number of elements in the \f$\CSA\f$.
        /*! Required for the Container Concept of the STL.
         *  \sa max_size, empty
         *  \par Time complexity
         *      \f$ \Order{1} \f$
         */
        size_type size()const
        {
            return m_psi.size();
        }

        //! Returns the largest size that csa_sada can ever have.
        /*! Required for the Container Concept of the STL.
         *  \sa size
         */
        static size_type max_size()
        {
            return t_enc_vec::max_size();
        }

        //! Returns if the data strucutre is empty.
        /*! Required for the Container Concept of the STL.A
         * \sa size
         */
        bool empty()const
        {
            return m_psi.empty();
        }

        //! Swap method for csa_sada
        /*! The swap method can be defined in terms of assignment.
            This requires three assignments, each of which, for a container type, is linear
            in the container's size. In a sense, then, a.swap(b) is redundant.
            This implementation guaranties a run-time complexity that is constant rather than linear.
            \param csa csa_sada to swap.

            Required for the Assignable Conecpt of the STL.
          */
        void swap(csa_sada_light& csa);

        //! Returns a const_iterator to the first element.
        /*! Required for the STL Container Concept.
         *  \sa end
         */
        const_iterator begin()const
        {
            return const_iterator(this, 0);
        }

        //! Returns a const_iterator to the element after the last element.
        /*! Required for the STL Container Concept.
         *  \sa begin.
         */
        const_iterator end()const
        {
            return const_iterator(this, size());
        }

        //! Assignment Copy Operator.
        /*!
         *    Required for the Assignable Concept of the STL.
         */
        csa_sada_light& operator=(const csa_sada_light& csa)
        {
            if (this != &csa) {
                copy(csa);
            }
            return *this;
        }

        //! Assignment Move Operator.
        /*!
         *    Required for the Assignable Concept of the STL.
         */
        csa_sada_light& operator=(csa_sada_light&& csa)
        {
            if (this != &csa) {
                m_psi        = std::move(csa.m_psi);
                m_alpha      = std::move(csa.m_alphabet);
                m_alphabet   = std::move(csa.m_alphabet);
                m_psi_buf    = std::move(csa.m_psi_buf);
            }
            return *this;
        }

        //! Serialize to a stream.
        /*! \param out Outstream to write the data structure.
         *  \return The number of written bytes.
         */
        size_type serialize(std::ostream& out, structure_tree_node* v=nullptr, std::string name="")const;

        //! Load from a stream.
        /*! \param in Input stream to load the data structure from.
         */
        void load(std::istream& in);

        uint32_t get_sample_dens() const
        {
            return t_dens;
        }

        size_type alpha() const {
            return m_alpha;
        }
};

// == template functions ==

template<class t_enc_vec, uint32_t t_dens, uint32_t t_inv_dens, class t_sa_sample_strat, class t_isa, class t_alphabet_strat>
csa_sada_light<t_enc_vec, t_dens, t_inv_dens, t_sa_sample_strat, t_isa, t_alphabet_strat>::csa_sada_light(cache_config& config)
{
    create_buffer();
    if (!cache_file_exists(key_trait<alphabet_type::int_width>::KEY_BWT, config)) {
        return;
    }
    int_vector_buffer<alphabet_type::int_width> bwt_buf(cache_file_name(key_trait<alphabet_type::int_width>::KEY_BWT,config));
    size_type n = bwt_buf.size();
    {
        auto event = memory_monitor::event("construct csa-alpbabet");
        alphabet_type tmp_alphabet(bwt_buf, n);
        m_alphabet.swap(tmp_alphabet);
    }

    int_vector<> cnt_chr(sigma, 0, bits::hi(n)+1);
    for (typename alphabet_type::sigma_type i=0; i < sigma; ++i) {
        cnt_chr[i] = C[i];
    }
    // calculate psi
    {
        auto event = memory_monitor::event("construct PSI");
        // TODO: move PSI construct into construct_PSI.hpp
        int_vector<> psi(n, 0, bits::hi(n)+1);

        for (size_type i=0; i < n; ++i) {
            psi[ cnt_chr[ char2comp[bwt_buf[i]] ]++ ] = i;
        }

        //strictly speaking, psi[0] is not defined, but due to the
        //rotation of the string for the BWT, psi[0] is precisely alpha
        //(with SA[alpha] = 0)
        m_alpha = psi[0];

        std::string psi_file = cache_file_name(conf::KEY_PSI, config);
        if (!store_to_cache(psi, conf::KEY_PSI, config)) {
            return;
        }
    }
    {
        auto event = memory_monitor::event("encode PSI");
        int_vector_buffer<> psi_buf(cache_file_name(conf::KEY_PSI, config));
        t_enc_vec tmp_psi(psi_buf);
        m_psi.swap(tmp_psi);
    }
}

template<class t_enc_vec, uint32_t t_dens, uint32_t t_inv_dens, class t_sa_sample_strat, class t_isa, class t_alphabet_strat>
auto csa_sada_light<t_enc_vec, t_dens, t_inv_dens, t_sa_sample_strat, t_isa, t_alphabet_strat>::serialize(std::ostream& out, structure_tree_node* v, std::string name)const -> size_type
{
    structure_tree_node* child = structure_tree::add_child(v, name, util::class_name(*this));
    size_type written_bytes = 0;
    written_bytes += m_psi.serialize(out, child, "psi");
    written_bytes += write_member(m_alpha, out, child, "alpha");
    written_bytes += m_alphabet.serialize(out, child, "alphabet");
    structure_tree::add_size(child, written_bytes);
    return written_bytes;
}

template<class t_enc_vec, uint32_t t_dens, uint32_t t_inv_dens, class t_sa_sample_strat, class t_isa, class t_alphabet_strat>
void csa_sada_light<t_enc_vec, t_dens, t_inv_dens, t_sa_sample_strat, t_isa, t_alphabet_strat>::load(std::istream& in)
{
    m_psi.load(in);
    read_member(m_alpha, in);
    m_alphabet.load(in);
}

template<class t_enc_vec, uint32_t t_dens, uint32_t t_inv_dens, class t_sa_sample_strat, class t_isa, class t_alphabet_strat>
void csa_sada_light<t_enc_vec, t_dens, t_inv_dens, t_sa_sample_strat, t_isa, t_alphabet_strat>::swap(csa_sada_light<t_enc_vec, t_dens, t_inv_dens, t_sa_sample_strat, t_isa, t_alphabet_strat>& csa)
{
    if (this != &csa) {
        m_psi.swap(csa.m_psi);
        std::swap(m_alpha, csa.m_alpha);
        m_alphabet.swap(csa.m_alphabet);
    }
}

} // end namespace sdsl
#endif
