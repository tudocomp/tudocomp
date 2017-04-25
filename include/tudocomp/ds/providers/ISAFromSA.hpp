#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/DSDef.hpp>
#include <tudocomp/ds/IntVector.hpp>

namespace tdc {

/// Constructs the inverse suffix array from the suffix array.
class ISAFromSA : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("provider", "isa");
        return m;
    }

private:
    DynamicIntVector m_isa;

public:
    using Algorithm::Algorithm;

    using provides = std::index_sequence<ds::INVERSE_SUFFIX_ARRAY>;
    using requires = std::index_sequence<ds::SUFFIX_ARRAY>;

    // implements concept "DSProvider"
    template<typename manager_t>
    inline void construct(manager_t& manager, bool compressed_space) {
        DLOG(INFO) << "ISAFromSA::construct";
    }

    // implements concept "DSProvider"
    template<dsid_t ds>
    inline void compress() {
        DLOG(INFO) << "ISAFromSA::compress<" << ds::name_for(ds) << ">";
    }

    // implements concept "DSProvider"
    template<dsid_t ds>
    inline void discard() {
        DLOG(INFO) << "ISAFromSA::discard<" << ds::name_for(ds) << ">";
    }

    // implements concept "DSProvider"
    using ds_types = tl::set<ds::INVERSE_SUFFIX_ARRAY, decltype(m_isa)>;

    template<dsid_t ds>
    const tl::get<ds, ds_types>& get();
};

template<>
const DynamicIntVector& ISAFromSA::get<ds::INVERSE_SUFFIX_ARRAY>() {
    return m_isa;
}

} //ns
