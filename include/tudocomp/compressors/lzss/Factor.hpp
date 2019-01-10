#pragma once

#include <tudocomp/def.hpp>

namespace tdc {
namespace lzss {

class Factor {
public:
    len_compact_t pos, src, len;

    inline Factor() {
        // undefined!
    }

    inline Factor(len_t fpos, len_t fsrc, len_t flen)
        : pos(fpos), src(fsrc), len(flen) {
    }
}  __attribute__((__packed__));

}}
