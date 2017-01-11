#pragma once

#include <tudocomp/Algorithm.hpp>

namespace tdc {
namespace lz78u {

class AsciiNt: public Algorithm {
private:

public:
    inline AsciiNt(Env&& env): Algorithm(std::move(env)) {}

    inline static Meta meta() {
        Meta m("lz78u_string_coder", "ascii_nt",
               "Codes string as a sequence of Ascii bytes terminated with a 0 byte.");
        return m;
    }


};

}
}//ns
