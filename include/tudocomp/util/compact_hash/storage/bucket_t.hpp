#pragma once

#include <memory>
#include <cstdint>
#include <utility>
#include <algorithm>

#include <tudocomp/util/bit_packed_layout_t.hpp>
#include <tudocomp/util/compact_hash/util.hpp>
#include <tudocomp/util/serialization.hpp>

namespace tdc {namespace compact_hash {
using namespace compact_hash;

/// A bucket of quotient-value pairs in a sparse compact hashtable.
///
/// It consists of a pointer to a single heap allocation, that contains:
/// - A 64-bit bitvector of currently stored elements.
/// - A dynamic-width array of quotients.
/// - A potentially dynamic-width array of satellite values.
///
/// An empty bucket does not allocate any memory.
///
/// WARNING:
/// To prevent the overhead of unnecessary default-constructions,
/// the bucket does not initialize or destroy the value and quotient parts
/// of the allocation in its constructor/destructor.
/// Instead, it relies on the surrounding container to initialize and destroy
/// the values correctly.
// TODO: Investigate changing this semantic to automatic initialization
// and destruction.
template<size_t N, typename satellite_t>
class bucket_t {
    std::unique_ptr<uint64_t[]> m_data;

    template<typename T>
    friend struct ::tdc::serialize;

    template<typename T>
    friend struct ::tdc::heap_size;

    using entry_ptr_t = typename satellite_t::entry_ptr_t;
    using entry_bit_width_t = typename satellite_t::entry_bit_width_t;
public:
    /// Maps hashtable position to position of the corresponding bucket,
    /// and the position inside of it.
    struct bucket_layout_t: satellite_t::bucket_data_layout_t {
        static constexpr size_t BVS_WIDTH_SHIFT = 6;
        static constexpr size_t BVS_WIDTH_MASK = 0b111111;

        static inline size_t table_pos_to_idx_of_bucket(size_t pos) {
            return pos >> BVS_WIDTH_SHIFT;
        }

        static inline size_t table_pos_to_idx_inside_bucket(size_t pos) {
            return pos & BVS_WIDTH_MASK;
        }

        static inline size_t table_size_to_bucket_size(size_t size) {
            return (size + BVS_WIDTH_MASK) >> BVS_WIDTH_SHIFT;
        }
    };

    inline bucket_t(): m_data() {}

    /// Construct a bucket, reserving space according to the bitvector
    /// `bv` and `quot_width`.
    inline bucket_t(uint64_t bv, entry_bit_width_t width) {
        if (bv != 0) {
            auto qvd_size = qvd_data_size(size(bv), width);

            m_data = std::make_unique<uint64_t[]>(qvd_size + 1);
            m_data[0] = bv;

            // NB: We call this for its alignment asserts
            ptr(width);
        } else {
            m_data.reset();
        }
    }

    inline bucket_t(bucket_t&& other) = default;
    inline bucket_t& operator=(bucket_t&& other) = default;

    /// Returns the bitvector of contained elements.
    inline uint64_t bv() const {
        if (!is_empty()) {
            return m_data[0];
        } else {
            return 0;
        }
    }

    /// Returns the amount of elements in the bucket.
    inline size_t size() const {
        return size(bv());
    }

    // Run destructors of each element in the bucket.
    inline void destroy_vals(entry_bit_width_t widths) {
        if (is_allocated()) {
            bucket_layout_t::destroy_vals(get_qv(), size(), widths);
        }
    }

    /// Returns a `entry_ptr_t` to position `pos`,
    /// or a sentinel value that acts as a one-pass-the-end pointer.
    inline entry_ptr_t at(size_t pos, entry_bit_width_t width) const {
        return bucket_layout_t::at(get_qv(), size(), pos, width);
    }

    inline bool is_allocated() const {
        return bool(m_data);
    }

    inline bool is_empty() const {
        return !bool(m_data);
    }

    inline size_t stat_allocation_size_in_bytes(entry_bit_width_t width) const {
        if (!is_empty()) {
            return (qvd_data_size(size(), width) + 1) * sizeof(uint64_t);
        } else {
            return 0;
        }
    }

    /// Insert a new element into the bucket, growing it as needed
    inline entry_ptr_t insert_at(
        size_t new_elem_bucket_pos,
        uint64_t new_elem_bv_bit,
        entry_bit_width_t width)
    {
        // Just a sanity check that can not live inside or outside `bucket_t` itself.
        static_assert(sizeof(bucket_t<N, satellite_t>) == sizeof(void*), "unique_ptr is more than 1 ptr large!");

        // TODO: check out different sizing strategies
        // eg, the known sparse_hash repo uses overallocation for small buckets

        // create a new bucket with enough size for the new element
        // NB: The elements in it are uninitialized
        auto new_bucket = bucket_t<N, satellite_t>(bv() | new_elem_bv_bit, width);

        auto new_iter = new_bucket.at(0, width);
        auto old_iter = at(0, width);

        auto const new_iter_midpoint = new_bucket.at(new_elem_bucket_pos, width);
        auto const new_iter_end = new_bucket.at(new_bucket.size(), width);

        entry_ptr_t ret;

        // move all elements before the new element's location from old bucket into new bucket
        while(new_iter != new_iter_midpoint) {
            new_iter.init_from(old_iter);
            new_iter.increment_ptr();
            old_iter.increment_ptr();
        }

        // move new element into its location in the new bucket
        {
            ret = new_iter;
            new_iter.increment_ptr();
        }

        // move all elements after the new element's location from old bucket into new bucket
        while(new_iter != new_iter_end) {
            new_iter.init_from(old_iter);
            new_iter.increment_ptr();
            old_iter.increment_ptr();
        }

        // destroy old empty elements, and overwrite with new bucket
        destroy_vals(width);
        *this = std::move(new_bucket);

        return ret;
    }
private:
    inline static size_t size(uint64_t bv) {
        return popcount(bv);
    }

    inline uint64_t* get_qv() const {
        return static_cast<uint64_t*>(m_data.get()) + 1;
    }

    inline static size_t qvd_data_size(size_t size, entry_bit_width_t width) {
        return bucket_layout_t::calc_sizes(size, width).overall_qword_size;
    }

    /// Creates the pointers to the beginnings of the two arrays inside
    /// the allocation.
    inline entry_ptr_t ptr(entry_bit_width_t width) const {
        return bucket_layout_t::ptr(get_qv(), size(), width);
    }
};

}

template<size_t N, typename satellite_t>
struct heap_size<compact_hash::bucket_t<N, satellite_t>> {
    using T = compact_hash::bucket_t<N, satellite_t>;
    using entry_bit_width_t = typename T::entry_bit_width_t;

    static object_size_t compute(T const& val, entry_bit_width_t const& widths) {
        using namespace compact_hash;

        auto bytes = object_size_t::empty();

        size_t size = val.size();

        if (size > 0) {
            size_t raw_size = T::qvd_data_size(size, widths) + 1;
            bytes += heap_size<std::unique_ptr<uint64_t[]>>::compute(val.m_data, raw_size);
        }

        return bytes;
    }
};

template<size_t N, typename satellite_t>
struct serialize<compact_hash::bucket_t<N, satellite_t>> {
    using T = compact_hash::bucket_t<N, satellite_t>;
    using entry_bit_width_t = typename T::entry_bit_width_t;

    static object_size_t write(std::ostream& out, T const& val, entry_bit_width_t const& widths) {
        using namespace compact_hash;

        auto bytes = object_size_t::empty();

        bytes += serialize<uint64_t>::write(out, val.bv());
        size_t size = val.size();

        if (size > 0) {
            size_t raw_size = T::qvd_data_size(size, widths) + 1;
            for (size_t i = 1; i < raw_size; i++) {
                bytes += serialize<uint64_t>::write(out, val.m_data[i]);
            }
        }

        return bytes;
    }
    static T read(std::istream& in, entry_bit_width_t const& widths) {
        using namespace compact_hash;

        T ret;

        uint64_t bv = serialize<uint64_t>::read(in);
        size_t size = T::size(bv);

        if (size > 0) {
            size_t raw_size = T::qvd_data_size(size, widths) + 1;
            ret.m_data = std::make_unique<uint64_t[]>(raw_size);
            ret.m_data[0] = bv;
            for (size_t i = 1; i < raw_size; i++) {
                ret.m_data[i] = serialize<uint64_t>::read(in);
            }
        }

        return ret;
    }
};

}
