#ifndef LZ77_IMPL_H
#define LZ77_IMPL_H

#include <tuple>

#include "esacomp/esacomp_rule_compressor.h"
#include "esacomp/rule.h"
#include "esacomp/rules.h"
#include "tudocomp.h"

namespace lz_compressor {

using namespace tudocomp;
using namespace esacomp;

class LZ77ClassicCompressor: public EsacompCompressStrategy {
public:
    using EsacompCompressStrategy::EsacompCompressStrategy;

    virtual Rules compress(Input& input, size_t threshold) final override;
};

}

#endif
