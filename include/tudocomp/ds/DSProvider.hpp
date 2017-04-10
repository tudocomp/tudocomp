#pragma once

#include <vector>

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/Env.hpp>

#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/DSDef.hpp>

namespace tdc {

/// Abstract base for text data structure providesr.
class DSProvider : public Algorithm {
public:
    inline virtual ~DSProvider() {
    }

    /// Constructor.
    using Algorithm::Algorithm;

    /// Gets the id list of the data structures required by this algorithm.
    virtual dsid_list_t requirements() const = 0;

    /// Gets the id list of the data structures computed by this algorithm.
    virtual dsid_list_t products() const = 0;

    /// Returns true iff this provider provides the specified data structure.
    inline bool does_provide(dsid_t what) const {
        for(auto dsid : products()) {
            if(dsid == what) return true;
        }
        return false;
    }
};

} //ns
