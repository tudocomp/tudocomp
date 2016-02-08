#ifndef DUMMY_CODER_H
#define DUMMY_CODER_H

#include "tudocomp.h"
#include "esacomp/esacomp_rule_compressor.h"
#include "esacomp/rule.h"
#include "esacomp/rules.h"

// Put every C++ code in this project into a common namespace
// in order to not pollute the global one
namespace esacomp {

using namespace tudocomp;

class DummyCoder: public EsacompEncodeStrategy {
public:
    using EsacompEncodeStrategy::EsacompEncodeStrategy;

    virtual void code(Rules&&, Input& input, Output&) override final;
    virtual void decode(Input& inp, Output& out) override final;
    virtual size_t min_encoded_rule_length(size_t input_size) override final;
};

}

#endif
