#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/DSDef.hpp>
#include <tudocomp/ds/IntVector.hpp>

namespace tdc {

/// Constructs the suffix array using divsufsort.
class DivSufSort : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("provider", "divsufsort");
        return m;
    }

private:
    DynamicIntVector m_sa;

public:
    using Algorithm::Algorithm;

    using provides = std::index_sequence<ds::SUFFIX_ARRAY>;
    using requires = std::index_sequence<>;
    using ds_types = tl::set<ds::SUFFIX_ARRAY, decltype(m_sa)>;

    // implements concept "DSProvider"
    template<typename manager_t>
    inline void construct(manager_t& manager, bool compressed_space) {
        DLOG(INFO) << "DivSufSort::construct";
    }

    // implements concept "DSProvider"
    template<dsid_t ds>
    inline void compress() {
        DLOG(INFO) << "DivSufSort::compress<" << ds::name_for(ds) << ">";
    }

    // implements concept "DSProvider"
    template<dsid_t ds>
    inline void discard() {
        DLOG(INFO) << "DivSufSort::discard<" << ds::name_for(ds) << ">";
    }

    // implements concept "DSProvider"
    template<dsid_t ds> const tl::get<ds, ds_types>& get();
    template<dsid_t ds> tl::get<ds, ds_types> relinquish();
};

template<>
const DynamicIntVector& DivSufSort::get<ds::SUFFIX_ARRAY>() {
    return m_sa;
}

template<>
DynamicIntVector DivSufSort::relinquish<ds::SUFFIX_ARRAY>() {
    return std::move(m_sa);
}

} //ns
