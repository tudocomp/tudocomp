#include "rule.h"
#include <cstdint>

namespace tudocomp {

std::ostream& operator<< (std::ostream& stream, const Rule& rule) {
    return stream << "("
           << rule.target << ", "
           << rule.source << ", "
           << rule.num << ")";
}

}
