#ifndef LZSS_FACTOR_H
#define LZSS_FACTOR_H

#include <cstdint>
#include <iostream>

namespace lz77rule {

/// A substitution rule.
struct Rule {
    /// Target location where that rule points to.
    size_t target;
    /// Source location where the rule takes the bytes from.
    size_t source;
    /// Size of the rule, that is number of bytes covered.
    size_t num;
};

inline std::ostream& operator<< (std::ostream& stream, const Rule& rule) {
    return stream << "("
           << rule.target << ", "
           << rule.source << ", "
           << rule.num << ")";
}

inline bool operator== (const Rule& left, const Rule& right) {
    return left.target == right.target
        && left.source == right.source
        && left.num == right.num;
}

inline bool operator!= (const Rule& left, const Rule& right) {
    return left.target != right.target
        || left.source != right.source
        || left.num != right.num;
}

/// Callable object, acts as a comparator of Rules, to order them
/// acording to target position.
struct rule_compare {
    inline bool operator() (const Rule& lhs, const Rule& rhs) const {
        return lhs.target < rhs.target;
    }
};

}

#endif
