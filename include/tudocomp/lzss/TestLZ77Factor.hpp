#ifndef _INCLUDED_LZ77FACTOR_HPP
#define _INCLUDED_LZ77FACTOR_HPP

#include <algorithm>
#include <vector>

namespace tudocomp {
namespace lzss {

/// Represents a factor according to LZ77.
/// Compression is achieved by replacing a phrase in the input by a reference
/// to another occurence of the same phrase.
class TestLZ77Factor {
    
public:
    /// The position at which this factor will be placed.
    size_t pos;
    
    /// The source position, which this factor refers to.
    size_t src;
    
    /// The amount of replaced symbols.
    size_t num;
    
    /// Default constructor.
    inline TestLZ77Factor() : pos(0), src(0), num(0) {
    }
    
    /// Constructor.
    inline TestLZ77Factor(size_t _pos, size_t _src, size_t _num) : pos(_pos), src(_src), num(_num) {
    }
    
    /// Comparison operator.
    /// The order of factors is given by their position.
    inline bool operator <(const TestLZ77Factor& other) const {
        return pos < other.pos;
    }
};

}}

#endif
