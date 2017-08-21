#pragma once

//#include <tudocomp/compressors/esp/>

namespace tdc {namespace esp {
    // TODO: Better memory layout
    struct TypedBlock {
        uint8_t len;
        uint8_t type;
    };
    bool operator==(const TypedBlock& a, const TypedBlock& b) {
        return a.len == b.len && a.type == b.type;
    }
    std::ostream& operator<<(std::ostream& o, const TypedBlock& b) {
        return o << "{ len: " << int(b.len) << ", type: " << int(b.type) << " }";
    }
}}
