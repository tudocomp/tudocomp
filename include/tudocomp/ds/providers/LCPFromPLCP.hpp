#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/DSDef.hpp>
#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/util.hpp>
#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {

/// Constructs the LCP array using the PLCP array.
class LCPFromPLCP : public Algorithm {
public:
    inline static Meta meta() {
        Meta m(ds::provider_type(), "lcp");
        return m;
    }

private:
    DynamicIntVector m_lcp;
    len_t m_max_lcp;

public:
    using Algorithm::Algorithm;

    using provides = std::index_sequence<ds::LCP_ARRAY>;
    using requires = std::index_sequence<ds::SUFFIX_ARRAY, ds::PLCP_ARRAY>;
    using ds_types = tl::set<ds::LCP_ARRAY, decltype(m_lcp)>;

    // implements concept "DSProvider"
    template<typename manager_t>
    inline void construct(manager_t& manager, bool compressed_space) {
        auto& t = manager.input;
        const size_t n = t.size();

        // get suffix and PLCP array
        auto& sa   = manager.template get<ds::SUFFIX_ARRAY>();
        auto& plcp = manager.template get<ds::PLCP_ARRAY>();

        m_max_lcp = manager.template get_provider<ds::PLCP_ARRAY>().max_lcp;

        StatPhase::wrap("Construct LCP", [&]{
            // Compute LCP array
            m_lcp = DynamicIntVector(
                n, 0, compressed_space ? bits_for(m_max_lcp) : INDEX_BITS);

            m_lcp[0] = 0;
            for(len_t i = 1; i < n; i++) {
                const len_t x = plcp[sa[i]];
                m_lcp[i] = x;
            }

            StatPhase::log("bit_width", size_t(m_lcp.width()));
            StatPhase::log("size", m_lcp.bit_size() / 8);
        });
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
inline void LCPFromPLCP::discard<ds::LCP_ARRAY>() {
    m_lcp.clear();
    m_lcp.shrink_to_fit();
}

template<>
inline void LCPFromPLCP::compress<ds::LCP_ARRAY>() {
    StatPhase::wrap("Compress LCP", [this]{
        m_lcp.width(bits_for(m_max_lcp));
        m_lcp.shrink_to_fit();

        StatPhase::log("bit_width", size_t(m_lcp.width()));
        StatPhase::log("size", m_lcp.bit_size() / 8);
    });
}

template<>
inline const DynamicIntVector& LCPFromPLCP::get<ds::LCP_ARRAY>() const {
    return m_lcp;
}

template<>
inline DynamicIntVector LCPFromPLCP::relinquish<ds::LCP_ARRAY>() {
    return std::move(m_lcp);
}

} //ns
