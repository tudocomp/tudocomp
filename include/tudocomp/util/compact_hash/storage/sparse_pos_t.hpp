#pragma once

#include <memory>
#include <cstdint>
#include <utility>
#include <algorithm>

#include <tudocomp/util/compact_hash/util.hpp>

namespace tdc {namespace compact_hash {

/// This type represents a position inside the compact sparse hashtable.
///
/// It is valid to have a sparse_pos_t one-past-the-end of the underlying
/// bucket vector, to act as an end-iterator.
template<typename bucket_t, typename bucket_layout_t>
class sparse_pos_t {
private:
    bucket_t* m_buckets;

public:
    /// Index of bucket inside the hashtable
    size_t idx_of_bucket;

    /// Bit mask of the element inside the bucket
    uint64_t bit_mask_in_bucket;

    inline sparse_pos_t(size_t pos, bucket_t* buckets):
        m_buckets(buckets),
        idx_of_bucket(bucket_layout_t::table_pos_to_idx_of_bucket(pos)),
        bit_mask_in_bucket(1ull << bucket_layout_t::table_pos_to_idx_inside_bucket(pos))
    {}

    inline sparse_pos_t(): m_buckets(nullptr) {}
    inline sparse_pos_t(sparse_pos_t const& other) = default;
    inline sparse_pos_t& operator=(sparse_pos_t const& other) = default;

    /// Accesses the bucket at this sparse position.
    inline bucket_t& bucket() const {
        //DCHECK_LT(idx_of_bucket, m_buckets->size());
        return m_buckets[idx_of_bucket];
    }

    /// Check if the sparse position exists in the corresponding bucket.
    inline bool exists_in_bucket() const {
        // bitvector of the bucket
        uint64_t bv = bucket().bv();

        return (bv & bit_mask_in_bucket) != 0;
    }

    /// Get the idx of the element inside the corresponding bucket.
    ///
    /// It is legal to call this method even if the element at
    /// the sparse position does not exists, to calculate a position
    /// at which it should be inserted.
    inline size_t offset_in_bucket() const {
        // bitvector of the bucket
        uint64_t bv = bucket().bv();

        return popcount(bv & (bit_mask_in_bucket - 1));
    }
};

}}
