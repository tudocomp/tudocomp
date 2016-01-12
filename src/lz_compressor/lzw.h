#ifndef LZW_H
#define LZW_H

#include <tuple>

#include "rule.h"
#include "tudocomp.h"
#include "lzwrule.h"

namespace lz_compressor {

using namespace tudocomp;
using namespace lzwrule;

class LZWCompressor: public LzwRuleCompressor {
public:
    inline LZWCompressor(Env& env): LzwRuleCompressor(env) {}

    virtual LzwEntries compress(const Input& input) final override;
};

class LZWDebugCode: public LzwRuleCoder {
public:
    inline LZWDebugCode(Env& env): LzwRuleCoder(env) {}

    virtual void code(LzwEntries entries, Input input, std::ostream& out) final override;
    virtual void decode(std::istream& inp, std::ostream& out) final override;
};

class LZWBitCode: public LzwRuleCoder {
public:
    inline LZWBitCode(Env& env): LzwRuleCoder(env) {}

    virtual void code(LzwEntries entries, Input input, std::ostream& out) final override;
    virtual void decode(std::istream& inp, std::ostream& out) final override;
};

}

#endif
