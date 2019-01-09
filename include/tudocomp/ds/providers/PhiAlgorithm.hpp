#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/DSDef.hpp>
#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/util.hpp>
#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {

/// Constructs the PLCP array using the Phi array.
class PhiAlgorithm : public Algorithm {
public:
    inline static Meta meta() {
        Meta m(ds::provider_type(), "phi_algorithm");
        return m;
    }

private:
    DynamicIntVector m_plcp;
    len_t m_max_lcp;

public:
    using Algorithm::Algorithm;

    using provides = std::index_sequence<ds::PLCP_ARRAY>;
    using requires = std::index_sequence<ds::PHI_ARRAY>;
    using ds_types = tl::set<ds::PLCP_ARRAY, decltype(m_plcp)>;

    // implements concept "DSProvider"
    template<typename manager_t>
    inline void construct(manager_t& manager, bool compressed_space) {
        auto& t = manager.input;
        const size_t n = t.size();

        // get Phi array for in-place construction
        m_plcp = manager.template inplace<ds::PHI_ARRAY>();

        StatPhase::wrap("Construct PLCP", [&]{
            // Use Phi algorithm to compute PLCP array
            m_max_lcp = 0;
            for(len_t i = 0, l = 0; i < n - 1; ++i) {
                const len_t phi_i = m_plcp[i];
                while(t[i + l] == t[phi_i + l]) ++l;
                m_max_lcp = std::max(m_max_lcp, l);
                m_plcp[i] = l;
                if(l) --l;
            }

            StatPhase::log("bit_width", size_t(m_plcp.width()));
            StatPhase::log("size", m_plcp.bit_size() / 8);
        });

        if(compressed_space) compress<ds::PLCP_ARRAY>();
    }

    // implements concept "DSProvider"
    template<dsid_t ds> void compress();
    template<dsid_t ds> void discard();
    template<dsid_t ds> const tl::get<ds, ds_types>& get() const;
    template<dsid_t ds> tl::get<ds, ds_types> relinquish();

    // implements concept "LCPInfo"
    const len_t& max_lcp = m_max_lcp;
};

template<>
inline void PhiAlgorithm::discard<ds::PLCP_ARRAY>() {
    m_plcp.clear();
    m_plcp.shrink_to_fit();
}

template<>
inline void PhiAlgorithm::compress<ds::PLCP_ARRAY>() {
    StatPhase::wrap("Compress PLCP", [this]{
        m_plcp.width(bits_for(m_max_lcp));
        m_plcp.shrink_to_fit();

        StatPhase::log("bit_width", size_t(m_plcp.width()));
        StatPhase::log("size", m_plcp.bit_size() / 8);
    });
}

template<>
const DynamicIntVector& PhiAlgorithm::get<ds::PLCP_ARRAY>() const {
    return m_plcp;
}

template<>
DynamicIntVector PhiAlgorithm::relinquish<ds::PLCP_ARRAY>() {
    return std::move(m_plcp);
}

} //ns
