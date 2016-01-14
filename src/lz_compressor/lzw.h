#ifndef LZW_H
#define LZW_H

#include <tuple>

#include "tudocomp.h"
#include "lzwrule.h"

namespace lz_compressor {

using namespace tudocomp;
using namespace lzwrule;

class LZWDebugCode: public LzwRuleCoder {
public:
    inline LZWDebugCode(Env& env): LzwRuleCoder(env) {}

    virtual void code(LzwEntries&& entries, Output& out) final override;
    virtual void decode(Input& inp, Output& out) final override;
};

class LZWBitCode: public LzwRuleCoder {
public:
    inline LZWBitCode(Env& env): LzwRuleCoder(env) {}

    virtual void code(LzwEntries&& entries, Output& out) final override;
    virtual void decode(Input& inp, Output& out) final override;
};

}

#endif
