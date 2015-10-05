#ifndef RULE_H
#define RULE_H

#include <cstdint>
#include <iostream>

namespace tudocomp {

/// A substitution rule.
struct Rule {
    /// Target location where that rule points to.
    size_t target;
    /// Source location where the rule takes the bytes from.
    size_t source;
    /// Size of the rule, that is number of bytes covered.
    size_t num;
};

std::ostream& operator<< (std::ostream& stream, const Rule& rule);
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

}

#endif
