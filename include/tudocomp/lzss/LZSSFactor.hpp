#ifndef _INCLUDED_LZSSFACTOR_HPP
#define _INCLUDED_LZSSFACTOR_HPP

#include <algorithm>
#include <vector>

namespace tudocomp {
namespace lzss {

/// Represents a LZSS factor.
class LZSSFactor {
    
public:
    size_t pos;
    size_t src;
    size_t num;
    
    inline LZSSFactor() : pos(0), src(0), num(0) {
    }
    
    inline LZSSFactor(size_t _pos, size_t _src, size_t _num) : pos(_pos), src(_src), num(_num) {
    }
    
    inline bool operator <(const LZSSFactor& other) const {
        return pos < other.pos;
    }
};

}}

#endif
