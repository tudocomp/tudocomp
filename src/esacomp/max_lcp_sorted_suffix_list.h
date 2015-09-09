#ifndef LCPSORTEDSUFFIXLIST_H
#define LCPSORTEDSUFFIXLIST_H

#include <cstdint>

#include "sdsl/int_vector.hpp"

#include "max_lcp.h"

namespace esacomp {

class MaxLCPSortedSuffixList: public MaxLCP {
public:
    MaxLCPSortedSuffixList(SdslVec, SdslVec, size_t);

    size_t getMaxEntry();
    size_t getEntryForSuffix(size_t suf);
    size_t getSize();
    size_t getSuffix(size_t i);
    size_t getSharedSuffix(size_t i);
    size_t getLCP(size_t i);
    void remove(size_t i);
    void updateLCP(size_t i, size_t newLCP);

    std::string toString();
private:
    //data backend
    size_t threshold;
    SdslVec sa;
    SdslVec lcp;

    //linked list
    std::vector<ssize_t> prev;
    std::vector<ssize_t> next;

    //linked list head (maximum LCP)
    ssize_t max = -1;

    //linked list tail (current minimum LCP)
    ssize_t last = -1;

    //number of entires
    size_t numEntries = 0;

    std::vector<ssize_t> lcpIndex;

    //suffix reference table
    std::vector<ssize_t> suffixRef;

    void insert(size_t i);
    ssize_t floorIndexEntry(size_t lcp);
};

}

#endif
