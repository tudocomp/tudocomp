#pragma once

#include <stdexcept>

namespace tdc {

class MissingSentinelError : public std::logic_error {
public:
    template<typename str_t>
    static inline void check(const str_t& str) {
        if(!str.ends_with(uint8_t(0))){
            throw MissingSentinelError();
        }
    }

    inline MissingSentinelError() : std::logic_error(
        "The input is missing a sentinel! If you are running tudocomp from "
        "the command line, pass the -0 parameter.") {
    }
};

}
