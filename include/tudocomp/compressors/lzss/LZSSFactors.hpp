#ifndef _INCLUDED_LZSS_FACTORS_HPP_
#define _INCLUDED_LZSS_FACTORS_HPP_

#include <tuple>
#include <sdsl/int_vector.hpp>

namespace tdc {
namespace lzss {

class Factor {
public:
    size_t pos, src, len;

    inline Factor(size_t fpos, size_t fsrc, size_t flen)
        : pos(fpos), src(fsrc), len(flen) {
    }
};

class FactorBuffer {
private:
    std::vector<Factor> m_factors;
    bool m_sorted;

public:
    inline FactorBuffer() : m_sorted(true) {}

    inline void push_back(Factor f) {
        m_sorted = m_sorted && (m_factors.empty() || f.pos >= m_factors.back().pos);
        m_factors.push_back(f);
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
};

}} //ns

#endif
