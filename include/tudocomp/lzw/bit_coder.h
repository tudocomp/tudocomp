#ifndef LZW_BIT_CODER_H
#define LZW_BIT_CODER_H

#include <tudocomp/lzw/lzw_compressor.h>

namespace lzw {

using namespace tudocomp;


class LZWBitCode: public LzwRuleCoder {
public:
    inline LZWBitCode(Env& env): LzwRuleCoder(env) {}

    virtual void code(LzwEntries&& entries, Output& out) final override;
    virtual void decode(Input& inp, Output& out) final override;
};

}

#endif
