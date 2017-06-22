#pragma once

#include <algorithm>
#include <vector>
#include <tudocomp/def.hpp>

namespace tdc {
namespace lzss {

class Factor {
public:
    len_compact_t pos, src, len;

    inline Factor(len_t fpos, len_t fsrc, len_t flen)
        : pos(fpos), src(fsrc), len(flen) {
    }
}  __attribute__((__packed__));


class FactorBuffer {
private:
    std::vector<Factor> m_factors;
    bool m_sorted; //! factors need to be sorted before they are output

    len_t m_shortest_factor;
    len_t m_longest_factor;

public:
    using const_iterator = std::vector<Factor>::const_iterator;

    inline FactorBuffer()
        : m_sorted(true)
        , m_shortest_factor(INDEX_MAX)
        , m_longest_factor(0)
    {
    }

    inline void emplace_back(len_t fpos, len_t fsrc, len_t flen) {
        m_sorted = m_sorted && (m_factors.empty() || fpos >= m_factors.back().pos);
        m_factors.emplace_back(fpos, fsrc, flen);

        m_shortest_factor = std::min(m_shortest_factor, flen);
        m_longest_factor = std::max(m_longest_factor, flen);
    }

    inline const_iterator begin() const {
        return m_factors.cbegin();
    }

    inline const_iterator end() const {
        return m_factors.cend();
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

