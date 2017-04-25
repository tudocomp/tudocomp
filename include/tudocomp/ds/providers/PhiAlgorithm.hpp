#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/DSDef.hpp>
#include <tudocomp/ds/IntVector.hpp>

namespace tdc {

/// Constructs the PLCP and LCP array using the Phi array.
class PhiAlgorithm : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("provider", "phi_algorithm");
        return m;
    }

private:
    DynamicIntVector m_plcp;
    DynamicIntVector m_lcp;

public:
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
    inline void compress() {
        DLOG(INFO) << "PhiAlgorithm::compress<" << ds::name_for(ds) << ">";
    }

    // implements concept "DSProvider"
    template<dsid_t ds>
    inline void discard() {
        DLOG(INFO) << "PhiAlgorithm::discard<" << ds::name_for(ds) << ">";
    }

    // implements concept "DSProvider"
    using ds_types = tl::mix<
        tl::set<ds::PLCP_ARRAY, decltype(m_plcp)>,
        tl::set<ds::LCP_ARRAY,  decltype(m_lcp)>
    >;

    template<dsid_t ds>
    const tl::get<ds, ds_types>& get();
};

template<>
const DynamicIntVector& PhiAlgorithm::get<ds::PLCP_ARRAY>() {
    return m_plcp;
}

template<>
const DynamicIntVector& PhiAlgorithm::get<ds::LCP_ARRAY>() {
    return m_lcp;
}

} //ns
