#ifndef LZ78_DUMMY_CODER_H
#define LZ78_DUMMY_CODER_H

#include <tudocomp/lz78/lz78_compressor.h>

namespace lz78 {

using namespace tudocomp;

class LZ78DebugCode: public Lz78RuleCoder {
public:
    inline LZ78DebugCode(Env& env): Lz78RuleCoder(env) {}

    virtual void code(Entries&& entries, Output& out) final override;
    virtual void decode(Input& inp, Output& out) final override;
};

}

#endif
