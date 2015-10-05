#ifndef CODE0_H
#define CODE0_H

#include "tudocomp.h"

namespace esacomp {

using namespace tudocomp;

class Code0Coder: public Coder {
public:
    virtual void code(Rules, Input input, std::ostream&) override final;
    virtual void decode(std::istream& inp, std::ostream& out) override final;
};

}

#endif
