#pragma once

#include <tudocomp/compressors/esp/GrammarRules.hpp>

namespace tdc {namespace esp {
    struct Round {
        GrammarRules gr;
        size_t alphabet;
        std::vector<size_t> string;
    };
}}
