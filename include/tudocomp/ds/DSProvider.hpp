#pragma once

#include <vector>
#include <tudocomp/util/type_list.hpp>

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Env.hpp>

#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/DSDef.hpp>

namespace tdc {

/// Abstract base for text data structure providers.
class DSProvider : public Algorithm {
public:
    inline virtual ~DSProvider() {
    }

    /// Constructor.
    using Algorithm::Algorithm;
};

} //ns
