#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/DSDef.hpp>

namespace tdc {

/// Constructs the PLCP and LCP array using the Phi array.
class PhiAlgorithm : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("provider", "phi_algorithm");
        return m;
    }

    using Algorithm::Algorithm;

    using provides = std::index_sequence<ds::PLCP_ARRAY, ds::LCP_ARRAY>;
    using requires = std::index_sequence<ds::PHI_ARRAY>;

    // implements concept "DSProvider"
    template<typename manager_t>
    inline void construct(manager_t& manager, bool compressed_space) {
        DLOG(INFO) << "PhiAlgorithm::construct";
    }

    // implements concept "DSProvider"
    template<dsid_t ds>
    inline void discard() {
        DLOG(INFO) << "PhiAlgorithm::discard<" << ds::name_for(ds) << ">";
    }
};

} //ns
