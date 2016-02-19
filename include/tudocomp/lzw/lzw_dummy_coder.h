#ifndef LZW_DUMMY_CODER_H
#define LZW_DUMMY_CODER_H

#include <tudocomp/lzw/lzw_compressor.h>

namespace lzw {

using namespace tudocomp;

class LZWDebugCode: public LzwRuleCoder {
public:
    inline LZWDebugCode(Env& env): LzwRuleCoder(env) {}

    virtual void code(LzwEntries&& entries, Output& out) final override;
    virtual void decode(Input& inp, Output& out) final override;
};

}

#endif
