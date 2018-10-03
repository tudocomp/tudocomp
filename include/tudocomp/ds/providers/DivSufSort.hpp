#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/DSDef.hpp>
#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/util/divsufsort.hpp>
#include <tudocomp/util.hpp>
#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {

/// Constructs the suffix array using divsufsort.
class DivSufSort : public Algorithm {
public:
    inline static Meta meta() {
        Meta m(ds::provider_type(), "divsufsort");
        return m;
    }

private:
    DynamicIntVector m_sa;

public:
    using Algorithm::Algorithm;

    using sa_t = decltype(m_sa);

    using provides = std::index_sequence<ds::SUFFIX_ARRAY>;
    using requires = std::index_sequence<>;
    using ds_types = tl::set<ds::SUFFIX_ARRAY, sa_t>;

    // implements concept "DSProvider"
    template<typename manager_t>
    inline void construct(manager_t& manager, bool compressed_space) {
        StatPhase::wrap("Construct SA", [&]{
            // Allocate
            const size_t n = manager.input.size();
            const size_t w = bits_for(n);

            // divsufsort needs one additional bit for signs
            m_sa = DynamicIntVector(
                n, 0, compressed_space ? w + 1 : INDEX_BITS);

            // Use divsufsort to construct
            divsufsort(manager.input.data(), m_sa, n);

            StatPhase::log("bit_width", size_t(m_sa.width()));
            StatPhase::log("size", m_sa.bit_size() / 8);
        });

        if(compressed_space) {
            // we can now drop the extra bit
            compress<ds::SUFFIX_ARRAY>();
        }
    }

    // implements concept "DSProvider"
    template<dsid_t ds> void compress();
    template<dsid_t ds> void discard();
    template<dsid_t ds> const tl::get<ds, ds_types>& get();
    template<dsid_t ds> tl::get<ds, ds_types> relinquish();
};

template<>
inline void DivSufSort::discard<ds::SUFFIX_ARRAY>() {
    m_sa.clear();
    m_sa.shrink_to_fit();
}

template<>
inline void DivSufSort::compress<ds::SUFFIX_ARRAY>() {
    StatPhase::wrap("Compress SA", [this]{
        m_sa.width(bits_for(m_sa.size()));
        m_sa.shrink_to_fit();

        StatPhase::log("bit_width", size_t(m_sa.width()));
        StatPhase::log("size", m_sa.bit_size() / 8);
    });
}

template<>
inline const DivSufSort::sa_t& DivSufSort::get<ds::SUFFIX_ARRAY>() {
    return m_sa;
}

template<>
inline DivSufSort::sa_t DivSufSort::relinquish<ds::SUFFIX_ARRAY>() {
    return std::move(m_sa);
}

} //ns
