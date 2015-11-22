#ifndef CODE1_H
#define CODE1_H

#include "tudocomp.h"
#include "lz77rule.h"

namespace esacomp {

using namespace lz77rule;
using namespace tudocomp;

class Code1Coder: public Lz77RuleCoder {
public:
    inline Code1Coder(Env& env): Lz77RuleCoder(env) {}

    virtual void code(Rules, Input input, std::ostream&) override final;
    virtual void decode(std::istream& inp, std::ostream& out) override final;
    virtual size_t min_encoded_rule_length(size_t input_size) override final;
};

}

#endif
