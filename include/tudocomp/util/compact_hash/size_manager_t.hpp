#pragma once

#include <cstdint>

#include "decomposed_key_t.hpp"

#include <tudocomp/util/serialization.hpp>
#include <tudocomp/util/heap_size.hpp>

namespace tdc {namespace compact_hash {

/// This manages the size of the hashtable, and related calculations.
class size_manager_t {
    /*
     * TODO: This is currently hardcoded to work with power-of-two table sizes.
     * Generalize it to allows arbitrary growth functions.
     */

    uint8_t m_capacity_log2;
    size_t m_size;
    float m_load_factor = 0.5;

    template<typename T>
    friend struct ::tdc::serialize;

    template<typename T>
    friend struct ::tdc::heap_size;

    /// Adjust the user-specified size of the table as needed
    /// by the current implementation.
    ///
    /// In this case, the grow function multiplies the capacity by two,
    /// so we need to start at a value != 0.
    inline static size_t adjust_size(size_t size) {
        return (size < 2) ? 2 : size;
    }

    size_manager_t() = default;

public:
    /// runtime initilization arguments, if any
    struct config_args {
        config_args() {}
        config_args(float load_factor): load_factor(load_factor) {}

        float load_factor = 0.5;
    };

    /// get the config of this instance
    inline config_args current_config() const {
        return config_args {
            m_load_factor,
        };
    }

    /// Create the size manager with an initial table size `capacity`
    inline size_manager_t(size_t capacity, config_args config = config_args{}) {
        capacity = adjust_size(capacity);

        m_size = 0;
        m_load_factor = config.load_factor;
        CHECK(is_pot(capacity));
        m_capacity_log2 = log2_upper(capacity);
    }

    /// Returns the amount of elements currently stored in the hashtable.
    inline size_t size() const {
        return m_size;
    }

    /// Update the amount of elements currently stored in the hashtable
    inline void set_size(size_t new_size) {
        DCHECK_LT(new_size, capacity());
        m_size = new_size;
    }

    /// The amount of bits used by the current table size.
    // TODO: Remove/make private
    inline uint8_t capacity_log2() const {
        return m_capacity_log2;
    }

    /// The current table size.
    inline size_t capacity() const {
        return 1ull << m_capacity_log2;
    }

    /// Check if the capacity needs to grow for the size given as the
    /// argument.
    inline bool needs_to_grow_capacity(size_t capacity, size_t new_size) const {
        // Capacity, at which a re-allocation is needed
        size_t trigger_capacity = size_t(float(capacity) * m_load_factor);

        // Make sure we have always a minimum of 1 free space in the table.
        trigger_capacity = std::min(capacity - 1, trigger_capacity);

        bool ret = trigger_capacity < new_size;
        return ret;
    }

    /// Returns the new capacity after growth.
    inline size_t grown_capacity(size_t capacity) const {
        DCHECK_GE(capacity, 1U);
        return capacity * 2;
    }

    /// Decompose the hash value such that `initial_address`
    /// covers the entire table, and `quotient` contains
    /// the remaining bits.
    inline decomposed_key_t decompose_hashed_value(uint64_t hres) {
        uint64_t shift = capacity_log2();

        return decomposed_key_t {
            hres & ((1ull << shift) - 1ull),
            hres >> shift,
        };
    }

    /// Composes a hash value from an `initial_address` and `quotient`.
    inline uint64_t compose_hashed_value(uint64_t initial_address, uint64_t quotient) {
        uint64_t shift = capacity_log2();
        uint64_t harg = (quotient << shift) | initial_address;
        return harg;
    }

    /// Adds the `add` value to `v`, and wraps it around the current capacity.
    template<typename int_t>
    inline int_t mod_add(int_t v, int_t add = 1) const {
        size_t mask = capacity() - 1;
        return (v + add) & mask;
    }

    /// Subtracts the `sub` value to `v`, and wraps it around the current capacity.
    template<typename int_t>
    inline int_t mod_sub(int_t v, int_t sub = 1) const {
        size_t mask = capacity() - 1;
        return (v - sub) & mask;
    }

    /// Sets the maximum load factor
    /// (how full the table can get before re-allocating).
    ///
    /// Expects a value `0.0 < z < 1.0`.
    inline void max_load_factor(float z) {
        DCHECK_GT(z, 0.0);
        DCHECK_LE(z, 1.0);
        m_load_factor = z;
    }

    /// Returns the maximum load factor.
    inline float max_load_factor() const noexcept {
        return m_load_factor;
    }
};

}

template<>
struct heap_size<compact_hash::size_manager_t> {
    using T = compact_hash::size_manager_t;

    static object_size_t compute(T const& val) {
        using namespace compact_hash;

        auto bytes = object_size_t::empty();

        bytes += heap_size<uint8_t>::compute(val.m_capacity_log2);
        bytes += heap_size<size_t>::compute(val.m_size);
        bytes += heap_size<float>::compute(val.m_load_factor);

        return bytes;
    }
};

template<>
struct serialize<compact_hash::size_manager_t> {
    using T = compact_hash::size_manager_t;

    static object_size_t write(std::ostream& out, T const& val) {
        using namespace compact_hash;

        auto bytes = object_size_t::empty();

        bytes += serialize<uint8_t>::write(out, val.m_capacity_log2);
        bytes += serialize<size_t>::write(out, val.m_size);
        bytes += serialize<float>::write(out, val.m_load_factor);

        return bytes;
    }
    static T read(std::istream& in) {
        using namespace compact_hash;

        T ret;
        ret.m_capacity_log2 = serialize<uint8_t>::read(in);
        ret.m_size = serialize<size_t>::read(in);
        ret.m_load_factor = serialize<float>::read(in);
        return ret;
    }
    static bool equal_check(T const& lhs, T const& rhs) {
        return gen_equal_check(m_capacity_log2)
        && gen_equal_check(m_size)
        && gen_equal_check(m_load_factor);
    }
};

}
