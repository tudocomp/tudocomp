#ifndef LZ78RULE_H
#define LZ78RULE_H

#include <cstdint>
#include <iostream>

#include <tudocomp/util.h>

namespace lz78 {

/// A lz78 dictonary entry
struct Entry {
    size_t index;
    uint8_t chr;
};

inline std::ostream& operator<< (std::ostream& stream, const Entry& rule) {
    stream << "(" << rule.index << ", ";
    return stream << tudocomp::byte_to_nice_ascii_char(rule.chr) << ")";
}

inline bool operator== (const Entry& left, const Entry& right) {
    return left.index == right.index
        && left.chr == right.chr;
}

inline bool operator!= (const Entry& left, const Entry& right) {
    return left.index != right.index
        || left.chr != right.chr;
}

}

#endif
