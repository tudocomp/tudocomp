#pragma once

#include <tudocomp/util/compact_hash/entry_t.hpp>

#include "val_quot_ptrs_t.hpp"
#include "val_quot_bucket_layout_t.hpp"

namespace tdc {namespace compact_hash{namespace map {

template<typename val_t>
struct satellite_data_t {
private:
    using qvd_t = val_quot_bucket_layout_t<val_t>;
    using widths_t = typename qvd_t::QVWidths;
public:
    static constexpr bool has_sentinel = true;
    using entry_ptr_t = val_quot_ptrs_t<val_t>;
    using entry_bit_width_t = widths_t;

    using bucket_data_layout_t = qvd_t;

    using sentinel_value_type = typename cbp::cbp_repr_t<val_t>::value_type;
};

}}}
