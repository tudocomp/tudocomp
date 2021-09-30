#pragma once

#include <tudocomp/ds/dynamic_t.hpp>
#include <tudocomp/ds/uint_t.hpp>
#include <tudocomp/util/metaprogramming.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/ds/IntRepr.hpp>
#include <type_traits>

namespace tdc {namespace cbp {

template< typename, typename = tdc::void_t<> >
struct has_IntRepr_t : std::false_type { };
template< typename T >
struct has_IntRepr_t<T, tdc::void_t<typename int_vector::IntRepr<T>::IntPtrBase>> : std::true_type { };
template< typename T >
constexpr bool has_IntRepr_v = has_IntRepr_t<T>::value;

template<typename T, typename = tdc::void_t<>>
struct cbp_repr_t{
};

template<typename T>
struct cbp_repr_t<T, std::enable_if_t<!has_IntRepr_v<T>>>{
    using value_type = T;
    class width_repr_t {
    public:
        constexpr width_repr_t(uint8_t) {}
        constexpr uint8_t get_width() const {
            return sizeof(T) * CHAR_BIT;
        }
        constexpr bool needs_alignment() const {
            return true;
        }
    };

    static constexpr uint8_t width_from_value(T const& value) {
        return 0;
    }

    using pointer_t = T*;
    using reference_t = T&;

#ifdef NDEBUG
#define CONSTEXPRONNDEBUG constexpr
#else
#define CONSTEXPRONNDEBUG
#endif

    static CONSTEXPRONNDEBUG pointer_t construct_relative_to(uint64_t* base_ptr,
                                                     uint64_t bit_offset,
                                                     uint64_t bit_element_size) {
#ifndef NDEBUG
        DCHECK_EQ(bit_offset % 8, 0);
        DCHECK_EQ(bit_element_size % 8, 0);
#endif
        auto offset = bit_offset / 8;
        return reinterpret_cast<T*>(reinterpret_cast<char*>(base_ptr) + offset);
    }
#undef CONSTEXPRONNDEBUG

    static inline void call_destructor(pointer_t p) {
        p->~T();
    }
    static inline void construct_val_from_ptr(pointer_t dst, pointer_t src) {
        new(dst) T(std::move(*src));
    }
    static inline void construct_val_from_rval(pointer_t dst, T&& src) {
        new(dst) T(std::move(src));
    }
};

template<typename T>
class cbp_repr_t<T, std::enable_if_t<has_IntRepr_v<T>>>{
    using WidthRepr = typename int_vector::IntRepr<T>::WidthRepr;
public:

    using value_type = typename int_vector::IntRepr<T>::value_type;
    class width_repr_t: WidthRepr {
    public:
        constexpr width_repr_t(uint8_t size): WidthRepr(size) {}
        constexpr uint8_t get_width() const {
            return this->data_bit_size();
        }
        constexpr bool needs_alignment() const {
            return false;
        }
    };

    static constexpr uint8_t width_from_value(value_type const& value) {
        return WidthRepr::width_from_value(value);
    }

    using pointer_t = int_vector::IntPtr<T>;
    using reference_t = int_vector::IntRef<T>;

    static constexpr pointer_t construct_relative_to(uint64_t* base_ptr,
                                                     uint64_t bit_offset,
                                                     uint64_t bit_element_size) {
        // Find the uint64_t in which the int pointer starts
        base_ptr += bit_offset / 64;
        bit_offset = bit_offset % 64;

        using ptr_base_t = int_vector::IntPtrBase<pointer_t>;

        auto int_ptr_base = ptr_base_t(base_ptr, bit_offset, bit_element_size);
        auto int_ptr = pointer_t(int_ptr_base);

        return int_ptr;
    }

    static inline void call_destructor(pointer_t p) {
        // NOP (also not implementable as is)
    }
    static inline void construct_val_from_ptr(pointer_t dst, pointer_t src) {
        *dst = *src;
    }
    static inline void construct_val_from_rval(pointer_t dst, value_type&& src) {
        *dst = src;
    }
};

template<typename T>
class cbp_sized_value_t: cbp_repr_t<T>::width_repr_t {
    using value_type = typename cbp_repr_t<T>::value_type;

    value_type m_value;
public:
    using width_t = typename cbp_repr_t<T>::width_repr_t;

    inline cbp_sized_value_t(value_type&& value):
        width_t(cbp_repr_t<T>::width_from_value(value)),
        m_value(std::move(value)) {
    }
    inline cbp_sized_value_t(value_type&& value, uint8_t width):
        width_t(width),
        m_value(std::move(value)) {
    }
    inline cbp_sized_value_t(value_type const& value):
        cbp_sized_value_t(value_type(value)) {}
    inline cbp_sized_value_t(value_type const& value, uint8_t width):
        cbp_sized_value_t(value_type(value), width) {}

    inline uint8_t width() const {
        return this->get_width();
    }
    inline value_type const& value() const {
        return m_value;
    }
    inline value_type& value() {
        return m_value;
    }
};

}}
