#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/DSDef.hpp>
#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/util.hpp>
#include <tudocomp_stat/StatPhase.hpp>

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
    using ds_types = tl::set<ds::PHI_ARRAY, decltype(m_phi)>;

    // implements concept "DSProvider"
    template<typename manager_t>
    inline void construct(manager_t& manager, bool compressed_space) {
        // get suffix array
        auto& sa = manager.get<ds::SUFFIX_ARRAY>();

        const size_t n = manager.input.size();
        const size_t w = bits_for(n);

        StatPhase::wrap("Construct Phi Array", [&]{
            // Construct Phi Array
            m_phi = DynamicIntVector(n, 0, compressed_space ? w : INDEX_BITS);

            for(len_t i = 1, prev = sa[0]; i < n; i++) {
                m_phi[sa[i]] = prev;
                prev = sa[i];
            }
            m_phi[sa[0]] = sa[n-1];

            StatPhase::log("bit_width", size_t(m_phi.width()));
            StatPhase::log("size", m_phi.bit_size() / 8);
        });
    }

    // implements concept "DSProvider"
    template<dsid_t ds> void compress();
    template<dsid_t ds> void discard();
    template<dsid_t ds> const tl::get<ds, ds_types>& get();
    template<dsid_t ds> tl::get<ds, ds_types> relinquish();
};

template<>
inline void PhiFromSA::discard<ds::PHI_ARRAY>() {
    m_phi.clear();
    m_phi.shrink_to_fit();
}

template<>
inline void PhiFromSA::compress<ds::PHI_ARRAY>() {
    StatPhase::wrap("Compress Phi Array", [this]{
        m_phi.width(bits_for(m_phi.size()));
        m_phi.shrink_to_fit();

        StatPhase::log("bit_width", size_t(m_phi.width()));
        StatPhase::log("size", m_phi.bit_size() / 8);
    });
}

template<>
const DynamicIntVector& PhiFromSA::get<ds::PHI_ARRAY>() {
    return m_phi;
}

template<>
DynamicIntVector PhiFromSA::relinquish<ds::PHI_ARRAY>() {
    return std::move(m_phi);
}

} //ns
