#ifndef LZ78RULE_H
#define LZ78RULE_H

#include <cstdint>
#include <iostream>

#include <tudocomp/util.h>
#include <tudocomp/lz78/dictionary.hpp>

namespace tudocomp {
namespace lz78 {

using lz78_dictionary::CodeType;

/// A lz78 dictonary entry
struct Factor {
    CodeType index;
    uint8_t chr;
};

inline std::ostream& operator<< (std::ostream& stream, const Factor& rule) {
    stream << "(" << rule.index << ", ";
    return stream << tudocomp::byte_to_nice_ascii_char(rule.chr) << ")";
}

inline bool operator== (const Factor& left, const Factor& right) {
    return left.index == right.index
        && left.chr == right.chr;
}

inline bool operator!= (const Factor& left, const Factor& right) {
    return left.index != right.index
        || left.chr != right.chr;
}

}
}

#endif
