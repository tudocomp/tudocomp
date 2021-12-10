#pragma once

#include "conditional_bit_packed.hpp"

namespace tdc {namespace cbp {

class bit_layout_element_t {
    uint64_t m_bit_offset = 0;
    uint64_t m_bit_size = 0;
    uint64_t m_bit_element_size = 0;

public:
    constexpr bit_layout_element_t() {}
    constexpr bit_layout_element_t(uint64_t bit_offset, uint64_t bit_size, uint64_t element_size):
        m_bit_offset(bit_offset),
        m_bit_size(bit_size),
        m_bit_element_size(element_size) {}

    constexpr uint64_t bit_offset() const { return m_bit_offset; }
    constexpr uint64_t bit_size() const { return m_bit_size; }
    constexpr uint64_t bit_element_size() const { return m_bit_element_size; }
};

template<typename T>
class cbp_layout_element_t: public bit_layout_element_t {
public:
    using pointer_t = typename cbp_repr_t<T>::pointer_t;

    constexpr pointer_t ptr_relative_to(uint64_t* base_ptr) const {
        return cbp_repr_t<T>::construct_relative_to(base_ptr, bit_offset(), bit_element_size());
    }

    using bit_layout_element_t::bit_layout_element_t;
    constexpr cbp_layout_element_t() {}
    constexpr cbp_layout_element_t(bit_layout_element_t&& base):
        bit_layout_element_t(std::move(base)) {}
};


class bit_layout_t {
    uint64_t m_bit_offset = 0;

public:
    constexpr bit_layout_element_t aligned_elements(size_t align, size_t byte_size, size_t n) {
        static_assert(CHAR_BIT == 8, "sanity check");

        size_t align_val = align * CHAR_BIT;
        size_t align_mask = align_val - 1;

        if ((m_bit_offset & align_mask) != 0) {
            uint64_t align_skip = align_val - (m_bit_offset & align_mask);
            m_bit_offset += align_skip;
        }

        uint64_t bit_offset = m_bit_offset;
        uint64_t bit_size = (8ull * byte_size * n);

        m_bit_offset += bit_size;

        return bit_layout_element_t(bit_offset, bit_size, 8ull * byte_size);
    }

    constexpr bit_layout_element_t bit_packed_elements(size_t bit_size, size_t n) {
        uint64_t rbit_offset = m_bit_offset;
        uint64_t rbit_size = bit_size * n;
        m_bit_offset += rbit_size;

        return bit_layout_element_t(rbit_offset, rbit_size, bit_size);
    }

    template<typename T>
    constexpr cbp_layout_element_t<T> aligned_elements(size_t n) {
        return aligned_elements(alignof(T), sizeof(T), n);
    }

    template<typename T>
    using width_t = typename cbp_repr_t<T>::width_repr_t;

    template<typename T>
    constexpr cbp_layout_element_t<T> cbp_elements(size_t n, width_t<T> const& meta) {
        if(meta.needs_alignment()) {
            return aligned_elements<T>(n);
        } else {
            size_t width = meta.get_width();
            return bit_packed_elements(width, n);
        }
    }

    constexpr size_t get_size_in_uint64_t_units() const {
        return (m_bit_offset + 63ull) / 64ull;
    }

};

}}
