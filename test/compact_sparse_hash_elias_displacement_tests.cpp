#include <gtest/gtest.h>

#include <cstdint>
#include <algorithm>

#include <tudocomp/util/compact_hash/map/typedefs.hpp>

template<typename val_t>
using COMPACT_TABLE = tdc::compact_hash::map::sparse_elias_hashmap_t<val_t>;

#include "compact_hash_tests.template.hpp"
