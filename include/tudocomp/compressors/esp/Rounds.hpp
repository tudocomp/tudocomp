#pragma once

#include <tudocomp/compressors/esp/GrammarRules.hpp>

namespace tdc {namespace esp {
    struct Round {
        GrammarRules gr;
        size_t alphabet;
        std::vector<size_t> string;
    };

    struct Rounds {
        std::vector<Round> rounds;
        size_t root_node;
        bool empty = true;
        SLP slp;
    };
}}
