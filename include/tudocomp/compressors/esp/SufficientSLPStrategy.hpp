#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/esp/SLPDepSort.hpp>

namespace tdc {namespace esp {
    class SufficientSLPStrategy: public Algorithm {
    public:
        inline static Meta meta() {
            Meta m("slp_strategy", "sufficient");
            return m;
        };

        using Algorithm::Algorithm;

        inline void encode(EspContext& context, SLP&& slp, Output& output) const {
            slp_dep_sort(slp);

        }

        inline SLP decode(Input& input) const {
            return SLP();
        }
    };
}}
