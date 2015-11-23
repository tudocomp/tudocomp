#ifndef RULE_H
#define RULE_H

#include <cstdint>
#include <iostream>
#include "tudocomp.h"

namespace lz78rule {

/// A lz78 dictonary entry
struct Entry {
    size_t index;
    uint8_t chr;
};

inline std::ostream& operator<< (std::ostream& stream, const Entry& rule) {
    stream << "(" << rule.index << ", ";
    return tudocomp::byte_to_lossy_ascii_char(rule.chr, stream) << ")";
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
