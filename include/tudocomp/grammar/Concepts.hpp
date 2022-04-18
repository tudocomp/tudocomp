#pragma once

#include <concepts>
#include <cstddef>
#include <string>

namespace tdc::grammar {

template<typename GrammarType>
concept Queryable = requires(GrammarType gr, size_t i) {
    { gr.at(i) } -> std::convertible_to<char>;
    { gr.substr(i, i) } -> std::convertible_to<std::string>;
};

} // namespace tdc::grammar
