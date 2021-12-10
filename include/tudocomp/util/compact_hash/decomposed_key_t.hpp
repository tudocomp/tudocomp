#pragma once

#include <cstdint>

namespace tdc {namespace compact_hash {
    struct decomposed_key_t {
        size_t initial_address;   // initial address of key in table
        uint64_t stored_quotient;   // quotient value stored in table
    };
}}
