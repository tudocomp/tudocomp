#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/DSDef.hpp>
#include <tudocomp/util/type_list.hpp>

namespace tdc {

/// Constructs the suffix array using divsufsort.
class DivSufSort : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("provider", "divsufsort");
        return m;
    }

    using Algorithm::Algorithm;

    using provides = std::index_sequence<ds::SUFFIX_ARRAY>;
    using requires = std::index_sequence<>;

    // implements concept "DSProvider"
    template<typename manager_t>
    inline void construct(manager_t& manager, bool compressed_space) {
        DLOG(INFO) << "DivSufSort::construct";
    }

    // implements concept "DSProvider"
    template<dsid_t ds>
    inline void discard() {
        DLOG(INFO) << "DivSufSort::discard<" << ds::name_for(ds) << ">";
    }
};

} //ns
