#ifndef SA_COMPRESSOR_H
#define SA_COMPRESSOR_H

#include <vector>
#include <cstdint>
#include <set>

#include "sdsl/int_vector.hpp"

#include "tudocomp.h"
#include "esacomp/rule.h"
#include "esacomp/rules.h"
#include "esacomp/esacomp_rule_compressor.h"

using namespace tudocomp;
using namespace esacomp;

namespace sa_compressor {

/// A Sdsl vector used fo suffix and lcp arrays.
using SdslVec = sdsl::int_vector<0>;

/// Holds a suffix array and a lcp array.
struct SuffixData {
    SdslVec sa;
    SdslVec lcp;
};

/// Allows implementing EsacompCompressStrategy in terms of a compress method
/// receiving a suffix and lcp array directly.
class SACompressor: public EsacompCompressStrategy {
public:
    using EsacompCompressStrategy::EsacompCompressStrategy;

    virtual Rules compress(Input& input, size_t threshold) final override;

    // TODO: visibility control better, eg protected

    virtual Rules compress(SdslVec sa, SdslVec lcp, size_t threshold) = 0;

    static SuffixData computeESA(Input& input);
};

}

#endif
