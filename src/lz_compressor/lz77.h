#ifndef LZ77_IMPL_H
#define LZ77_IMPL_H

#include <tuple>

#include "lz77rule.h"
#include "rule.h"
#include "rules.h"
#include "tudocomp.h"

namespace lz_compressor {

using namespace tudocomp;
using namespace lz77rule;

class LZ77ClassicCompressor: public Lz77RuleCompressor {
public:
    using Lz77RuleCompressor::Lz77RuleCompressor;

    virtual Rules compress(Input& input, size_t threshold);
};

}

#endif
