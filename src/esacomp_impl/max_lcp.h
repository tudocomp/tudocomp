#ifndef MAXLCPIF_H
#define MAXLCPIF_H

#include <string>

#include "esacomp/rule.h"
#include "tudocomp.h"
#include "sa_compressor.h"

namespace esacomp {

class MaxLCP {
public:
    virtual ~MaxLCP() = default;

    // TODO: Pick proper integer types

    virtual size_t getMaxEntry() = 0;
    virtual size_t getEntryForSuffix(size_t suf) = 0;
    virtual size_t getSize() = 0;
    virtual size_t getSuffix(size_t i) = 0;
    virtual size_t getSharedSuffix(size_t i) = 0;
    virtual size_t getLCP(size_t i) = 0;
    virtual void remove(size_t i) = 0;
    virtual void updateLCP(size_t i, size_t newLCP) = 0;

    virtual std::string toString() = 0;
};

}

#endif
