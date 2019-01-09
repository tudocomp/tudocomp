#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/DSDef.hpp>
#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/util.hpp>
#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {

/// Constructs the inverse suffix array from the suffix array.
class ISAFromSA : public Algorithm {
public:
    inline static Meta meta() {
        Meta m(ds::provider_type(), "isa");
        return m;
    }

private:
    DynamicIntVector m_isa;

public:
    using Algorithm::Algorithm;

    using provides = std::index_sequence<ds::INVERSE_SUFFIX_ARRAY>;
    using requires = std::index_sequence<ds::SUFFIX_ARRAY>;
    using ds_types = tl::set<ds::INVERSE_SUFFIX_ARRAY, decltype(m_isa)>;

    // implements concept "DSProvider"
    template<typename manager_t>
    inline void construct(manager_t& manager, bool compressed_space) {
        // get suffix array
        auto& sa = manager.template get<ds::SUFFIX_ARRAY>();

        StatPhase::wrap("Construct ISA", [&]{
            // Allocate
            const size_t n = manager.input.size();
            const size_t w = bits_for(n);

            m_isa = DynamicIntVector(n, 0, compressed_space ? w : INDEX_BITS);

            // Construct
            for(len_t i = 0; i < n; i++) {
                m_isa[sa[i]] = i;
            }

            StatPhase::log("bit_width", size_t(m_isa.width()));
            StatPhase::log("size", m_isa.bit_size() / 8);
        });
    }

    // implements concept "DSProvider"
    template<dsid_t ds> void compress();
    template<dsid_t ds> void discard();
    template<dsid_t ds> const tl::get<ds, ds_types>& get() const;
    template<dsid_t ds> tl::get<ds, ds_types> relinquish();
};

template<>
inline void ISAFromSA::discard<ds::INVERSE_SUFFIX_ARRAY>() {
    m_isa.clear();
    m_isa.shrink_to_fit();
}

template<>
inline void ISAFromSA::compress<ds::INVERSE_SUFFIX_ARRAY>() {
    StatPhase::wrap("Compress ISA", [this]{
        m_isa.width(bits_for(m_isa.size()));
        m_isa.shrink_to_fit();

        StatPhase::log("bit_width", size_t(m_isa.width()));
        StatPhase::log("size", m_isa.bit_size() / 8);
    });
}

template<>
const DynamicIntVector& ISAFromSA::get<ds::INVERSE_SUFFIX_ARRAY>() const {
    return m_isa;
}

template<>
DynamicIntVector ISAFromSA::relinquish<ds::INVERSE_SUFFIX_ARRAY>() {
    return std::move(m_isa);
}

} //ns
