#ifndef _INCLUDED_MAX_LCP_SUFFIX_LIST_HPP_
#define _INCLUDED_MAX_LCP_SUFFIX_LIST_HPP_

#include <sdsl/int_vector.hpp>
#include <tudocomp/util.h>

namespace tudocomp {

/// Provides constant-time access to the suffix in a suffix array
/// with the longest correspondig lcp table entry.
///
/// Addition and removal are achieved in near-constant time.
/// (Dinklage, 2015).
template<typename S, typename L>
class MaxLCPSuffixList {

private:
    //data backend
    const S* m_sa;
    const L* m_lcp;

    //undefined suffix
    size_t m_undef;

    //linked list
    size_t m_first; //linked list head (maximum LCP)
    size_t m_last; //linked list tail (current minimum LCP)

    sdsl::int_vector<> m_prev;
    sdsl::int_vector<> m_next;

    //LCP index
    sdsl::int_vector<> m_lcp_index;

    //Suffix membership list
    sdsl::bit_vector m_suffix_contained;

    //Entry counter
    size_t m_size = 0;

    /// Lookup the LCP index.
    inline size_t lookup_lcp_index(size_t lcp) const {
        //DLOG(INFO) << "lookup_lcp_index(" << lcp << ")";
        DCHECK(lcp <= m_lcp_index.size());

        size_t result = m_undef;
        while(lcp > 0 && result == m_undef) {
            result = m_lcp_index[--lcp];
        }

        return result;
    }

public:
    /// Constructor
    inline MaxLCPSuffixList(const S& sa, const L& lcp, size_t min_lcp)
        : m_sa(&sa), m_lcp(&lcp) {

        size_t n = sa.size();
        m_undef = n;

        //Initialize doubly linked list
        m_first = m_undef;
        m_last = m_undef;
        m_prev = sdsl::int_vector<>(n, m_undef, bitsFor(m_undef));
        m_next = sdsl::int_vector<>(n, m_undef, bitsFor(m_undef));

        m_lcp_index = sdsl::int_vector<>(lcp.max_lcp(), m_undef, bitsFor(m_undef));

        //Initialize suffix reference map
        m_suffix_contained = sdsl::bit_vector(n, 0);

        //Construct list
        for (size_t i = 0; i < n; i++) {
            if (lcp[i] >= min_lcp) {
                insert(i);
            }
        }
    }

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
    inline size_t first() const {
        DCHECK(m_size > 0);
        return m_first;
    }
};

}

#endif
