#ifndef _INCLUDED_LZSS_FACTORS_HPP_
#define _INCLUDED_LZSS_FACTORS_HPP_

#include <tuple>
#include <sdsl/int_vector.hpp>

namespace tdc {
namespace lzss {

class Factor {
public:
    len_t pos, src, len;

    inline Factor(len_t fpos, len_t fsrc, len_t flen)
        : pos(fpos), src(fsrc), len(flen) {
    }
};

class FactorBuffer {
private:
    std::vector<Factor> m_factors;
    bool m_sorted;

    len_t m_shortest_factor;
    len_t m_longest_factor;

public:
    inline FactorBuffer() : m_sorted(true),
                            m_shortest_factor(SIZE_MAX),
                            m_longest_factor(0)
    {
    }

    inline void push_back(Factor f) {
        m_sorted = m_sorted && (m_factors.empty() || f.pos >= m_factors.back().pos);
        m_factors.push_back(f);

        m_shortest_factor = std::min(m_shortest_factor, f.len);
        m_longest_factor = std::max(m_longest_factor, f.len);
    }

    inline const Factor& operator[](size_t i) const {
        return m_factors[i];
    }

    inline bool empty() const {
        return m_factors.empty();
    }

    inline size_t size() const {
        return m_factors.size();
    }

    inline bool is_sorted() const {
        return m_sorted;
    }

    inline void sort() { //TODO: use radix sort
        if(!m_sorted) {
            std::sort(m_factors.begin(), m_factors.end(),
                [](const Factor& a, const Factor& b) -> bool { return a.pos < b.pos; });

            m_sorted = true;
        }
    }

    inline size_t shortest_factor() const {
        return m_shortest_factor;
    }

    inline size_t longest_factor() const {
        return m_longest_factor;
    }
};

}} //ns

#endif
