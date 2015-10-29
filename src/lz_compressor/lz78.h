#ifndef LZ78_H
#define LZ78_H

#include <tuple>

#include "rule.h"
#include "tudocomp.h"

using namespace tudocomp;

namespace lz_compressor {

class LZ78Compressor: public Lz77RuleCompressor {
public:
    inline LZ78Compressor(Env& env): Lz77RuleCompressor(env) {}

    virtual Rules compress(const Input& input, size_t threshold) final override;
};

}

#endif
