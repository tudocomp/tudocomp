#ifndef _INCLUDED_LZSS_FACTORS_HPP_
#define _INCLUDED_LZSS_FACTORS_HPP_

#include <tuple>

namespace tdc {
namespace lzss {

using Factor = std::tuple<size_t, size_t, size_t>;

class FactorBuffer {
private:
    size_t m_size;
    std::vector<size_t> m_pos, m_src, m_len;

public:
    inline FactorBuffer() : m_size(0) {}

    inline void push_back(Factor f) {
        size_t pos, src, len;
        std::tie(pos, src, len) = f;

        m_pos.push_back(pos);
        m_src.push_back(src);
        m_len.push_back(len);
        m_size++;
    }

    inline Factor operator[](size_t i) const {
        return Factor(m_pos[i], m_src[i], m_len[i]);
    }

    inline size_t pos(size_t i) const {
        return m_pos[i];
    }

    inline size_t src(size_t i) const {
        return m_src[i];
    }

    inline size_t len(size_t i) const {
        return m_len[i];
    }

    inline size_t size() const {
        return m_size;
    }
};

}} //ns

#endif
