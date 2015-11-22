#ifndef LZ78_H
#define LZ78_H

#include <tuple>

#include "rule.h"
#include "tudocomp.h"
#include "lz77rule.h"

namespace lz_compressor {

using namespace tudocomp;
using namespace lz77rule;

class LZ78Compressor: public Lz77RuleCompressor {
public:
    inline LZ78Compressor(Env& env): Lz77RuleCompressor(env) {}

    virtual Rules compress(const Input& input, size_t threshold) final override;
};

}

#endif
