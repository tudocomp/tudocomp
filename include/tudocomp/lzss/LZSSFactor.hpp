#ifndef _INCLUDED_LZSSFACTOR_HPP
#define _INCLUDED_LZSSFACTOR_HPP

#include <algorithm>
#include <vector>

namespace tudocomp {
namespace lzss {

class LZSSFactor {
    
public:
    size_t pos;
    size_t src;
    size_t num;
    
    inline LZSSFactor(size_t _pos, size_t _src, size_t _num) : pos(_pos), src(_src), num(_num) {}
};

}}

#endif
