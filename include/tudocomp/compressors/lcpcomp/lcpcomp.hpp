#pragma once

#include <tudocomp/meta/TypeDesc.hpp>

namespace tdc {
namespace lcpcomp {

inline constexpr TypeDesc comp_strategy_type() {
    return TypeDesc("lcpcomp_comp");
}

inline constexpr TypeDesc dec_strategy_type() {
    return TypeDesc("lcpcomp_dec");
}

}} //ns

