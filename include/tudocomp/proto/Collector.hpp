#ifndef _INCLUDED_COLLECTOR_HPP
#define _INCLUDED_COLLECTOR_HPP

#include <vector>

namespace tudocomp {

/// A consumer that stores items in a vector.
template<typename T>
struct Collector {
    std::vector<T> vector;
    
    inline void operator()(const T& item) {
        vector.push_back(item);
    }
};

}

#endif
