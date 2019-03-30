#pragma once

namespace tdc {namespace lz_common {

/// Type for the factor indices, bounded by the number of LZ trie nodes
using factorid_t = uint32_t;

/// Id that can be used for a non-existing factor
constexpr factorid_t undef_id = std::numeric_limits<factorid_t>::max();

/// Maximum legal dictionary size.
constexpr size_t DMS_MAX = std::numeric_limits<factorid_t>::max();

}}
