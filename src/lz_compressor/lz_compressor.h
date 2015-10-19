#ifndef LZCOMPRESSOR_H
#define LZCOMPRESSOR_H

#include "rule.h"
#include "tudocomp.h"
#include "sa_compressor.h"

using namespace sa_compressor;

namespace lz_compressor {

class LZCompressor: public SACompressor {
public:
    inline LZCompressor(Env& env): SACompressor(env) {}

    virtual Rules compress(SdslVec sa, SdslVec lcp, size_t threshold) override final;

    // If you wonder why this is here, google "C++ name hiding"
    using SACompressor::compress;
};

}

#endif
