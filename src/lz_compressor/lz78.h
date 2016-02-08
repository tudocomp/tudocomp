#ifndef LZ78_H
#define LZ78_H

#include <tuple>

#include "tudocomp.h"
#include "lz78rule.h"

namespace lz_compressor {

using namespace tudocomp;
using namespace lz78rule;

class LZ78DebugCode: public Lz78RuleCoder {
public:
    inline LZ78DebugCode(Env& env): Lz78RuleCoder(env) {}

    virtual void code(Entries&& entries, Output& out) final override;
    virtual void decode(Input& inp, Output& out) final override;
};

class LZ78BitCode: public Lz78RuleCoder {
public:
    inline LZ78BitCode(Env& env): Lz78RuleCoder(env) {}

    virtual void code(Entries&& entries, Output& out) final override;
    virtual void decode(Input& inp, Output& out) final override;
};

}

#endif
