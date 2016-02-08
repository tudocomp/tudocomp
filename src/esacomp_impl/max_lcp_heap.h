#ifndef LCPHEAP_H
#define LCPHEAP_H

#include <vector>

#include "sdsl/int_vector.hpp"

#include "max_heap.h"
#include "max_lcp.h"
#include "sa_compressor.h"

namespace esacomp {

class MaxLCPHeap: public MaxHeap<size_t>, public MaxLCP {
private:
    size_t threshold;
    size_t size;

    SdslVec sa;
    SdslVec lcp;
    // sa and lcpaccess offset
    size_t offset;

    std::vector<size_t> heap;
    std::vector<ssize_t> suffixRef;


    virtual void setSize(size_t size) override;
    virtual size_t getKey(size_t i) override;
    virtual void setKey(size_t i, size_t newKey) override;
    virtual void swap(size_t a, size_t b) override;
public:
    MaxLCPHeap(SdslVec sa, SdslVec lcp, size_t threshold);

    virtual size_t getSize() override;
    virtual size_t get(size_t i) override;

    virtual size_t getMaxEntry() override;
    virtual size_t getEntryForSuffix(size_t suf) override;
    virtual size_t getSuffix(size_t i) override;
    virtual size_t getSharedSuffix(size_t i) override;
    virtual size_t getLCP(size_t i) override;
    virtual void remove(size_t i) override;
    virtual void updateLCP(size_t i, size_t newLCP) override;

    virtual std::string toString() override;

    virtual void decreaseKey(size_t i, size_t newKey) override;
    virtual size_t removeFromHeap(size_t i) override;

};

}

#endif
