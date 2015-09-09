#include "rule.h"
#include <cstdint>

namespace tudocomp {

std::ostream& operator<< (std::ostream& stream, const Rule& rule) {
    return stream << "("
           << rule.target << ", "
           << rule.source << ", "
           << rule.num << ")";
}

bool operator== (const Rule& left, const Rule& right) {
    return left.target == right.target
        && left.source == right.source
        && left.num == right.num;
}

}
