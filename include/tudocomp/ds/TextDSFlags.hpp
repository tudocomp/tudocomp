#pragma once

#include <tudocomp/util.hpp>

namespace tdc {
namespace ds {
    using dsflags_t = unsigned int;

    constexpr dsflags_t NONE = 0x00;
    constexpr dsflags_t SA   = 0x01;
    constexpr dsflags_t ISA  = 0x02;
    constexpr dsflags_t LCP  = 0x04;
    constexpr dsflags_t PHI  = 0x08;
    constexpr dsflags_t PLCP = 0x10;
}
}
