#ifndef DUMMY_CODER_H
#define DUMMY_CODER_H

#include "tudocomp.h"
#include "lz77rule.h"
using namespace lz77rule;

// Put every C++ code in this project into a common namespace
// in order to not pollute the global one
namespace dummy {

using namespace tudocomp;

class DummyCoder: public Lz77RuleCoder {
public:
    inline DummyCoder(Env& env): Lz77RuleCoder(env) {}

    virtual void code(Rules, Input input, std::ostream&) override final;
    virtual void decode(std::istream& inp, std::ostream& out) override final;
    virtual size_t min_encoded_rule_length(size_t input_size) override final;
};

}

#endif
