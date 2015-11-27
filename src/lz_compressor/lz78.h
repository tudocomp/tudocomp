#ifndef LZ78_H
#define LZ78_H

#include <tuple>

#include "rule.h"
#include "tudocomp.h"
#include "lz78rule.h"

namespace lz_compressor {

using namespace tudocomp;
using namespace lz78rule;

class LZ78Compressor: public Lz78RuleCompressor {
public:
    inline LZ78Compressor(Env& env): Lz78RuleCompressor(env) {}

    virtual Entries compress(const Input& input) final override;
};

class LZWCompressor: public Lz78RuleCompressor {
public:
    inline LZWCompressor(Env& env): Lz78RuleCompressor(env) {}

    virtual Entries compress(const Input& input) final override;
};

class LZ78DebugCode: public Lz78RuleCoder {
public:
    inline LZ78DebugCode(Env& env): Lz78RuleCoder(env) {}

    virtual void code(Entries entries, Input input, std::ostream& out) final override;
    virtual void decode(std::istream& inp, std::ostream& out) final override;
};

class LZ78BitCode: public Lz78RuleCoder {
public:
    inline LZ78BitCode(Env& env): Lz78RuleCoder(env) {}

    virtual void code(Entries entries, Input input, std::ostream& out) final override;
    virtual void decode(std::istream& inp, std::ostream& out) final override;
};

}

#endif
