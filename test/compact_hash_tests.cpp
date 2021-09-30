#include <gtest/gtest.h>

#include <cstdint>
#include <algorithm>

#include <tudocomp/util/compact_hash/map/typedefs.hpp>

template<typename val_t>
using COMPACT_TABLE = tdc::compact_hash::map::plain_cv_hashmap_t<val_t>;

#include "compact_hash_tests.template.hpp"
