#pragma once

#include <tudocomp/util/compact_hash/entry_t.hpp>
#include "quot_ptr_t.hpp"
#include "quot_bucket_layout_t.hpp"

namespace tdc {namespace compact_hash {namespace set {

struct no_satellite_data_t {
    using entry_ptr_t = quot_ptr_t;
    using entry_bit_width_t = uint8_t;
    using bucket_data_layout_t = quot_bucket_layout_t;
    using sentinel_value_type = void;
};

}}}
