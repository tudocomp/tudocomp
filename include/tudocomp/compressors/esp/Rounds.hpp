#pragma once

#include <tudocomp/compressors/esp/GrammarRules.hpp>

namespace tdc {namespace esp {
    template<typename ipd_t>
    struct Round {
        GrammarRules<ipd_t> gr;
        size_t alphabet;
        IntVector<dynamic_t> string;
    };
}}
