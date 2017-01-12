#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/ds/IntVector.hpp>

namespace tdc {
namespace esacomp {

/// Provides constant-time access to the suffix in a suffix array
/// with the longest correspondig lcp table entry.
///
/// Addition and removal are achieved in near-constant time.
/// (Dinklage, 2015).
template<class lcp_t>
class MaxLCPSuffixList {

private:
    //data backend
    const lcp_t* m_lcp;

    //undefined suffix
    const size_t m_undef; // == m_lcp->size()

    //linked list
    size_t m_first; //linked list head (maximum LCP)
    size_t m_last; //linked list tail (current minimum LCP)

    DynamicIntVector m_prev, m_next;

    //LCP index
    DynamicIntVector m_lcp_index;

    //Suffix membership list
    BitVector m_suffix_contained;

    //Entry counter
    size_t m_size = 0;

    /// Lookup the LCP index.
    inline size_t lookup_lcp_index(size_t lcp) const {
        //DLOG(INFO) << "lookup_lcp_index(" << lcp << ")";
        DCHECK_LE(lcp, m_lcp_index.size());

        size_t result = m_undef;
        while(lcp > 0 && result == m_undef) {
            result = m_lcp_index[--lcp];
        }

        return result;
    }

public:
    /// Constructor
    inline MaxLCPSuffixList(const lcp_t& lcp, size_t min_lcp, size_t max_lcp)
        : m_lcp(&lcp), m_undef(m_lcp->size())
	{
        const size_t& n = lcp.size();

        //Initialize doubly linked list
        m_first = m_undef;
        m_last = m_undef;
        m_prev = DynamicIntVector(n, m_undef, bits_for(m_undef));
        m_next = DynamicIntVector(n, m_undef, bits_for(m_undef));

        m_lcp_index = DynamicIntVector(max_lcp, m_undef, bits_for(m_undef));

        //Initialize suffix reference map
        m_suffix_contained = BitVector(n, 0);

        //Construct list
        for (size_t i = 1; i < n; i++) {
            if (lcp[i] >= min_lcp) {
                insert(i);
            }
        }
    }

private:
    /// Insert suffix array item with index i.
    inline void insert(size_t i) {
        DCHECK(i < m_undef && !m_suffix_contained[i]);

        size_t lcp = (*m_lcp)[i];
        size_t pos = lookup_lcp_index(lcp);
        if(pos == m_undef) {
            //insert at end
            if(m_last != m_undef) {
                m_next[m_last] = i;
            }

            m_next[i] = m_undef;
            m_prev[i] = m_last;

            m_last = i;
        } else {
            //insert at position
            size_t prev = m_prev[pos];

            m_prev[i] = prev;
            m_next[i] = pos;

            if(prev != m_undef) {
                m_next[prev] = i;
            } else {
                DCHECK(pos == m_first);
                m_first = i;
            }

            m_prev[pos] = i;
        }

        //update lcp index
        m_lcp_index[lcp-1] = i;

        //update first
        if(m_first == m_undef) {
            m_first = i;
        }

        //update info
        m_suffix_contained[i] = 1;
        ++m_size;
    }

public:
    /// Remove suffix array item with index i.
    inline void remove(size_t i) {
        DCHECK(i < m_undef && m_suffix_contained[i]);

        //unlink
        if(m_prev[i] != m_undef) {
            m_next[m_prev[i]] = m_next[i];
        } else {
            DCHECK(i == m_first);
            m_first = m_next[i];
        }

        if(m_next[i] != m_undef) {
            m_prev[m_next[i]] = m_prev[i];
        } else {
            DCHECK(i == m_last);
            m_last = m_prev[i];
        }

        //update LCP index
        size_t lcp = (*m_lcp)[i];
        if(m_lcp_index[lcp-1] == i) {
            size_t k = m_next[i];
            if (k != m_undef && (*m_lcp)[k] == lcp) {
                m_lcp_index[lcp-1] = k; //move to next entry with same LCP
            } else {
                m_lcp_index[lcp-1] = m_undef; //invalidate
            }
        }

        //update info
        m_suffix_contained[i] = 0;
        --m_size;
    }

    /// Decrease key on array item with index i.
    inline void decrease_key(len_t i) {
        remove(i);
        insert(i);
    }

    /// Checks whether or not suffix array entry i is contained in this list.
    inline bool contains(size_t i) const {
        return m_suffix_contained[i];
    }

    /// Get number of contained entries.
    inline size_t size() const {
        return m_size;
    }

    /// Test if list is empty
    inline bool empty() const {
        return (m_size == 0);
    }

    /// Get first item (suffix array index with highest LCP)
    inline size_t get_max() const {
        DCHECK(m_size > 0);
        return m_first;
    }
};

}} //ns

