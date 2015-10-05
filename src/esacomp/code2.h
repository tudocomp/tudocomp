#ifndef CODE2_H
#define CODE2_H

#include "tudocomp.h"

namespace esacomp {

using namespace tudocomp;

class Code2Coder: public Coder {
public:
    virtual void code(Rules, Input input, std::ostream&) override final;
    virtual void decode(std::istream& inp, std::ostream& out) override final;
};

}

#endif
