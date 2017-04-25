#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/DSDef.hpp>
#include <tudocomp/ds/IntVector.hpp>

namespace tdc {

/// Constructs the Phi array from the suffix array.
class PhiFromSA : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("provider", "phi");
        return m;
    }

private:
    DynamicIntVector m_phi;

public:
    using Algorithm::Algorithm;

    using provides = std::index_sequence<ds::PHI_ARRAY>;
    using requires = std::index_sequence<ds::SUFFIX_ARRAY>;

    // implements concept "DSProvider"
    template<typename manager_t>
    inline void construct(manager_t& manager, bool compressed_space) {
        DLOG(INFO) << "PhiFromSA::construct";
    }

    // implements concept "DSProvider"
    template<dsid_t ds>
    inline void compress() {
        DLOG(INFO) << "PhiFromSA::compress<" << ds::name_for(ds) << ">";
    }

    // implements concept "DSProvider"
    template<dsid_t ds>
    inline void discard() {
        DLOG(INFO) << "PhiFromSA::discard<" << ds::name_for(ds) << ">";
    }

    // implements concept "DSProvider"
    using ds_types = tl::set<ds::PHI_ARRAY, decltype(m_phi)>;

    template<dsid_t ds>
    const tl::get<ds, ds_types>& get();
};

template<>
const DynamicIntVector& PhiFromSA::get<ds::PHI_ARRAY>() {
    return m_phi;
}

} //ns
