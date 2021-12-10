#pragma once

#include <tudocomp/util/compact_hash/set/hashset_t.hpp>
#include <tudocomp/util/compact_hash/hash_functions.hpp>
#include <tudocomp/util/compact_hash/index_structure/cv_bvs_t.hpp>
#include <tudocomp/util/compact_hash/index_structure/displacement_t.hpp>
#include <tudocomp/util/compact_hash/index_structure/elias_gamma_displacement_table_t.hpp>
#include <tudocomp/util/compact_hash/index_structure/layered_displacement_table_t.hpp>
#include <tudocomp/util/compact_hash/index_structure/naive_displacement_table_t.hpp>

namespace tdc {namespace compact_hash {namespace set {

template<typename hash_t = poplar_xorshift_t>
using sparse_cv_hashset_t
    = hashset_t<hash_t, cv_bvs_t>;

template<typename hash_t = poplar_xorshift_t>
using sparse_layered_hashset_t
    = hashset_t<hash_t, displacement_t<layered_displacement_table_t<dynamic_layered_bit_width_t>>>;

template<typename hash_t = poplar_xorshift_t>
using sparse_elias_hashset_t
    = hashset_t<hash_t, displacement_t<elias_gamma_displacement_table_t<
        dynamic_fixed_elias_gamma_bucket_size_t>>>;

}}}
