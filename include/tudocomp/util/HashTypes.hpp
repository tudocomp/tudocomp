#pragma once

#include <tudocomp/meta/TypeDesc.hpp>

namespace tdc {

constexpr TypeDesc hash_map_type() {
    return TypeDesc("hash");
}

constexpr TypeDesc hash_function_type() {
    return TypeDesc("hash_function");
}

constexpr TypeDesc hash_manager_type() {
    return TypeDesc("hash_manager");
}

constexpr TypeDesc hash_prober_type() {
    return TypeDesc("hash_prober");
}

constexpr TypeDesc hash_roller_type() {
    return TypeDesc("hash_roller");
}

} //ns

