#pragma once

#include <tudocomp/util/compact_hash/map/hashmap_t.hpp>
#include <tudocomp/util/compact_hash/hash_functions.hpp>
#include <tudocomp/util/compact_hash/storage/buckets_bv_t.hpp>
#include <tudocomp/util/compact_hash/storage/plain_sentinel_t.hpp>
#include <tudocomp/util/compact_hash/index_structure/cv_bvs_t.hpp>
#include <tudocomp/util/compact_hash/index_structure/displacement_t.hpp>
#include <tudocomp/util/compact_hash/index_structure/elias_gamma_displacement_table_t.hpp>
#include <tudocomp/util/compact_hash/index_structure/layered_displacement_table_t.hpp>
#include <tudocomp/util/compact_hash/index_structure/naive_displacement_table_t.hpp>

namespace tdc {namespace compact_hash {namespace map {

template<typename val_t, typename hash_t = poplar_xorshift_t>
using plain_cv_hashmap_t
    = hashmap_t<val_t, hash_t, plain_sentinel_t, cv_bvs_t>;

template<typename val_t, typename hash_t = poplar_xorshift_t>
using sparse_cv_hashmap_t
    = hashmap_t<val_t, hash_t, buckets_bv_t, cv_bvs_t>;

template<typename val_t, typename hash_t = poplar_xorshift_t>
using plain_layered_hashmap_t
    = hashmap_t<
        val_t, hash_t, plain_sentinel_t,
        displacement_t<layered_displacement_table_t<dynamic_layered_bit_width_t>>>;

template<typename val_t, typename hash_t = poplar_xorshift_t>
using sparse_layered_hashmap_t
    = hashmap_t<
        val_t, hash_t, buckets_bv_t,
        displacement_t<layered_displacement_table_t<dynamic_layered_bit_width_t>>>;

template<typename val_t, typename hash_t = poplar_xorshift_t>
using plain_elias_hashmap_t
    = hashmap_t<
        val_t, hash_t, plain_sentinel_t,
        displacement_t<elias_gamma_displacement_table_t<
            dynamic_fixed_elias_gamma_bucket_size_t>>>;

template<typename val_t, typename hash_t = poplar_xorshift_t>
using sparse_elias_hashmap_t
    = hashmap_t<
        val_t, hash_t, buckets_bv_t,
        displacement_t<elias_gamma_displacement_table_t<
            dynamic_fixed_elias_gamma_bucket_size_t>>>;

}}}
