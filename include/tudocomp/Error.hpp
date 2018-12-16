#pragma once

#include <stdexcept>

namespace tdc {

class MissingSentinelError : public std::logic_error {
public:
    inline MissingSentinelError() : std::logic_error(
        "The input is missing a sentinel! If you are running tudocomp from "
        "the command line, pass the -0 parameter.") {
    }
};

}
