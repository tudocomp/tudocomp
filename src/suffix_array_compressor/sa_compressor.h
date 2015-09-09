#ifndef SA_COMPRESSOR_H
#define SA_COMPRESSOR_H

#include <vector>
#include <cstdint>
#include <set>

#include "sdsl/int_vector.hpp"

#include "rule.h"
#include "tudocomp.h"

using namespace tudocomp;

namespace sa_compressor {

/// Callable object, acts as a comparator of Rules, to order them
/// acording to target position.
struct rule_compare {
    bool operator() (const Rule& lhs, const Rule& rhs) const {
        return lhs.target < rhs.target;
    }
};

/// A Sdsl vector used fo suffix and lcp arrays.
using SdslVec = sdsl::int_vector<0>;

/// Holds a suffix array and a lcp array.
struct SuffixData {
    SdslVec sa;
    SdslVec lcp;
};

/// Allows implementing Compressor in terms of a compress method
/// receiving a suffix and lcp array directly.
class SACompressor: public Compressor {
public:
    virtual Rules compress(const Input& input, size_t threshold) final override;

    // TODO: visibility controll better, eg protected

    virtual Rules compress(SdslVec sa, SdslVec lcp, size_t threshold) = 0;

    static SuffixData computeESA(const Input& input);
};

}

#endif
