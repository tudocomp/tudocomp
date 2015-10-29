#ifndef CODE2_H
#define CODE2_H

#include "tudocomp.h"

namespace esacomp {

using namespace tudocomp;

class Code2Coder: public Lz77RuleCoder {
public:
    inline Code2Coder(Env& env): Lz77RuleCoder(env) {}

    virtual void code(Rules, Input input, std::ostream&) override final;
    virtual void decode(std::istream& inp, std::ostream& out) override final;
    virtual size_t min_encoded_rule_length(size_t input_size) override final;
};

}

#endif
