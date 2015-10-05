#include "dummy_encoder.h"

// Put every C++ code in this project into a common namespace
// in order to not pollute the global one
namespace dummy {

void DummyCoder::code(Rules rules, Input input, std::ostream& out) {
    out.write((const char*) input.data(), input.size());
}

void DummyCoder::decode(std::istream& inp, std::ostream& out) {
    char c;
    while(inp.get(c)) {
        out.put(c);
    }
}

}
