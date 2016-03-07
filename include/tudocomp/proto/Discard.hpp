#ifndef _INCLUDED_DISCARD_HPP
#define _INCLUDED_DISCARD_HPP

#include <vector>

namespace tudocomp {

/// A simple consumer that discards anything given to it.
template<typename T>
struct Discard {
    inline void operator()(const T& item) {
    }
};

}

#endif
