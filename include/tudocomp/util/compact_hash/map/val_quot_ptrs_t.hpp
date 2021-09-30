#pragma once

#include <memory>
#include <cstdint>
#include <utility>
#include <algorithm>

#include <tudocomp/util/compact_hash/util.hpp>
#include <tudocomp/util/compact_hash/entry_t.hpp>

namespace tdc {namespace compact_hash{namespace map {

/// Represents a pair of pointers to value and quotient inside a bucket.
template<typename val_t>
class val_quot_ptrs_t {
    ValPtr<val_t> m_val_ptr;
    mutable QuotPtr m_quot_ptr;

public:
    using value_type = typename cbp::cbp_repr_t<val_t>::value_type;

    struct my_value_type {
        uint64_t quot;
        value_type val;
    };

    inline val_quot_ptrs_t(ValPtr<val_t> val_ptr,
                      QuotPtr quot_ptr):
        m_val_ptr(val_ptr),
        m_quot_ptr(quot_ptr)
    {
    }

    inline val_quot_ptrs_t():
        m_val_ptr(), m_quot_ptr() {}

    inline uint64_t get_quotient() const {
        return uint64_t(*m_quot_ptr);
    }

    inline void set_quotient(uint64_t v) const {
        *m_quot_ptr = v;
    }

    inline void swap_quotient(uint64_t& other) const {
        uint64_t tmp = uint64_t(*m_quot_ptr);
        std::swap(other, tmp);
        *m_quot_ptr = tmp;
    }

    inline ValPtr<val_t> val_ptr() const {
        return m_val_ptr;
    }

    inline QuotPtr quot_ptr() const {
        return m_quot_ptr;
    }

    inline void increment_ptr() {
        m_quot_ptr++;
        m_val_ptr++;
    }
    inline void decrement_ptr() {
        m_quot_ptr--;
        m_val_ptr--;
    }

    inline friend bool operator==(val_quot_ptrs_t const& lhs,
                                  val_quot_ptrs_t const& rhs)
    {
        return lhs.m_val_ptr == rhs.m_val_ptr;
    }

    inline friend bool operator!=(val_quot_ptrs_t const& lhs,
                                  val_quot_ptrs_t const& rhs)
    {
        return lhs.m_val_ptr != rhs.m_val_ptr;
    }

    inline void set(value_type&& val,
                    uint64_t quot) {
        set_quotient(quot);
        *val_ptr() = std::move(val);
    }

    inline void set_no_drop(value_type&& val,
                            uint64_t quot) {
        set_quotient(quot);
        cbp::cbp_repr_t<val_t>::construct_val_from_rval(val_ptr(), std::move(val));
    }

    inline void set_val(value_type&& val) {
        *val_ptr() = std::move(val);
    }

    inline void set_val_no_drop(value_type&& val) {
        cbp::cbp_repr_t<val_t>::construct_val_from_rval(val_ptr(), std::move(val));
    }

    inline void move_from(val_quot_ptrs_t other) {
        *val_ptr() = std::move(*other.val_ptr());
        set_quotient(other.get_quotient());
    }

    inline void init_from(val_quot_ptrs_t other) {
        cbp::cbp_repr_t<val_t>::construct_val_from_ptr(val_ptr(), other.val_ptr());
        set_quotient(other.get_quotient());
    }

    inline void swap_with(val_quot_ptrs_t other) {
        value_type tmp_val = std::move(*val_ptr());
        uint64_t tmp_quot = get_quotient();

        move_from(other);
        other.set(std::move(tmp_val), tmp_quot);
    }

    inline void uninitialize() {
        cbp::cbp_repr_t<val_t>::call_destructor(val_ptr());
    }

    inline bool contents_eq(val_quot_ptrs_t rhs) const {
        return (get_quotient() == rhs.get_quotient()) && (*val_ptr() == *rhs.val_ptr());
    }

    inline my_value_type move_out() const {
        return my_value_type {
            get_quotient(),
            std::move(*val_ptr()),
        };
    }

    inline void set(my_value_type&& val) {
        set_quotient(val.quot);
        *val_ptr() = std::move(val.val);
    }
};

}}}
