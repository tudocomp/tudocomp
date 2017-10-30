#pragma once

#include <tudocomp/util/View.hpp>
#include <tudocomp/compressors/esp/SLP.hpp>
#include <tudocomp/compressors/esp/Rounds.hpp>
#include <tudocomp/compressors/esp/utils.hpp>

namespace tdc {namespace esp {
    template<typename ipd_t>
    class EspContext {
    public:
        IPDStats ipd_stats;

        EspContext() = default;

        template<typename T>
        SLP generate_grammar(T&& s);
    };
}}
