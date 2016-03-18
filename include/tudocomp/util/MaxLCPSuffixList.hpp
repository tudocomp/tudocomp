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
    
    //minimum LCP for a suffix to be included in the list
    size_t m_min_lcp;
    
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
        DLOG(INFO) << "lookup_lcp_index(" << lcp << ")";
        assert(lcp <= m_lcp_index.size());

        size_t result = m_undef;
        while(lcp > 0 && result == m_undef) {
            result = m_lcp_index[--lcp];
        }
        
        return result;
    }

public:
    /// Constructor
    inline MaxLCPSuffixList(const S& sa, const L& lcp, size_t min_lcp)
        : m_sa(&sa), m_lcp(&lcp), m_min_lcp(min_lcp) {
            
        size_t n = sa.size();
        m_undef = n;
        
        //Initialize doubly linked list
        m_first = m_undef;
        m_last = m_undef;
        m_prev = sdsl::int_vector<>(n, m_undef, bitsFor(m_undef));
        m_next = sdsl::int_vector<>(n, m_undef, bitsFor(m_undef));
        
        //Initialize LCP index
        size_t max_lcp = 0;
        for (size_t i = 0; i < lcp.size(); i++) {
            if (lcp[i] > max_lcp) {
                max_lcp = lcp[i];
            }
        }

        m_lcp_index = sdsl::int_vector<>(max_lcp, m_undef, bitsFor(m_undef));
        
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
        assert(i < m_undef && !m_suffix_contained[i]);
        
        size_t lcp = (*m_lcp)[i];
        assert(lcp >= m_min_lcp);
        
        size_t pos = lookup_lcp_index(lcp);
        if(pos == m_undef) {
            //insert at end
            if(m_last != m_undef) {
                m_next[m_last] = i;
            }
            
            m_prev[i] = m_last;
            m_last = i;
        } else {
            //insert at position
            size_t old_prev = m_prev[pos];
            
            m_prev[pos] = i;
            if(old_prev != m_undef) {
                m_next[old_prev] = i;
            }
            
            m_prev[i] = old_prev;
            m_next[i] = pos;
        }
        
        //update lcp index
        m_lcp_index[lcp-1] = i;
        
        //update first
        if(m_first == m_undef || lcp >= (*m_lcp)[m_first]) {
            m_first = i;
        }
        
        //update info
        m_suffix_contained[i] = 1;
        ++m_size;
    }
    
    /// Remove suffix array item with index i.
    inline void remove(size_t i) {
        assert(i < m_undef && m_suffix_contained[i]);
        
        //unlink
        if(m_prev[i] != m_undef) {
            m_next[m_prev[i]] = m_next[i];
        }
        
        if(m_next[i] != m_undef) {
            m_prev[m_next[i]] = m_prev[i];
        }
        
        //update first and last
        if(m_first == i) {
            m_first = m_next[i];
        }
        
        if(m_last == i) {
            m_last = m_prev[i];
        }
        
        //update LCP index
        size_t lcp = (*m_lcp)[i] - 1;
        if(m_lcp_index[lcp] == i) {
            size_t k = m_next[i];
            if (k != m_undef && (*m_lcp)[k] == lcp) {
                m_lcp_index[lcp] = k; //move to next entry with same LCP
            } else {
                m_lcp_index[lcp] = m_undef; //invalidate
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
        assert(m_size > 0);
        return m_first;
    }
};

}

#endif
