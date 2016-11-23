#ifndef LZ78RULE_H
#define LZ78RULE_H

#include <cstdint>
#include <iostream>

#include <tudocomp/util.hpp>
#include <tudocomp/compressors/lz78/LZ78Dictionary.hpp>

namespace tdc {
namespace lz78 {

/// A lz78 dictonary entry
struct Factor {
    CodeType index;
    uliteral_t chr;
};

inline std::ostream& operator<< (std::ostream& stream, const Factor& rule) {
    stream << "(" << rule.index << ", ";
    return stream << tdc::byte_to_nice_ascii_char(rule.chr) << ")";
}

inline bool operator== (const Factor& left, const Factor& right) {
    return left.index == right.index
        && left.chr == right.chr;
}

inline bool operator!= (const Factor& left, const Factor& right) {
    return left.index != right.index
        || left.chr != right.chr;
}

}} //ns

#endif
