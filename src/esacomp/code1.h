#ifndef CODE1_H
#define CODE1_H

#include "tudocomp.h"

namespace esacomp {

using namespace tudocomp;

class Code1Coder: public Coder {
public:
    virtual void code(Rules, Input input, std::ostream&) override final;
    virtual void decode(std::istream& inp, std::ostream& out) override final;
};

}

#endif
