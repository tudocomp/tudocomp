#ifndef CODE0_H
#define CODE0_H

#include "tudocomp.h"
#include "esacomp/esacomp_rule_compressor.h"

namespace esacomp {

using namespace tudocomp;

class Code0Coder: public EsacompEncodeStrategy {
public:
    inline Code0Coder(Env& env): EsacompEncodeStrategy(env) {}

    virtual void code(Rules&&, Input& input, Output&) override final;
    virtual void decode(Input& inp, Output& out) override final;
    virtual size_t min_encoded_rule_length(size_t input_size) override final;
};

}

#endif
