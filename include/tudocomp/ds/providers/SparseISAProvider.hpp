#pragma once

#include <assert.h>
#include <type_traits>

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/DSDef.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/ds/Rank.hpp>

#include <tudocomp/util.hpp>
#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {

/// Constructs the inverse suffix array using the suffix array.
template<typename sa_provider_t>
class SparseISAProvider : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("provider", "sparse_isa");
        m.option("t").dynamic(3);
        return m;
    }

private:
    using this_t = SparseISAProvider<sa_provider_t>;

    using sa_t = tl::get<ds::SUFFIX_ARRAY, typename sa_provider_t::ds_types>;
    const sa_t* m_sa;

    BitVector m_has_shortcut;
    Rank      m_rank;

    DynamicIntVector m_shortcuts;

public:
    using Algorithm::Algorithm;

    using provides = std::index_sequence<ds::INVERSE_SUFFIX_ARRAY>;
    using requires = std::index_sequence<ds::SUFFIX_ARRAY>;
    using ds_types = tl::set<ds::INVERSE_SUFFIX_ARRAY, this_t>;

    // implements concept "DSProvider"
    template<typename manager_t>
    inline void construct(manager_t& manager, bool compressed_space) {
        // Require Suffix Array
        m_sa = &manager.get<ds::SUFFIX_ARRAY>();

        const size_t n = m_sa->size();
        m_has_shortcut = BitVector(n);

        const size_t t = this->env().option("t").as_integer();

        // Construct
        StatPhase::wrap("Construct sparse ISA", [&]{
            auto v = BitVector(n);
            for(size_t i = 0; i < n; i++) {
                if(!v[i]) {
                    // new cycle
                    v[i] = 1;
                    size_t j = (*m_sa)[i];
                    size_t k = 1;

                    while(j != i) {
                        if((k % t) == 0) {
                            m_has_shortcut[j] = 1;
                        }

                        v[j] = 1;
                        j = (*m_sa)[j];
                        ++k;
                    }

                    if(k > t) m_has_shortcut[i] = 1;
                }
            }

            m_rank = Rank(m_has_shortcut);
            m_shortcuts = DynamicIntVector(m_rank(n-1), 0, bits_for(n));

            for(size_t i = 0; i < n; i++) {
                if(v[i]) {
                    v[i] = 0;
                    size_t j = (*m_sa)[i];
                    while(v[j]) {
                        if(m_has_shortcut[j]) {
                            m_shortcuts[m_rank(j)-1] = i;
                            i = j;
                        }
                        v[j] = 0;
                        j = (*m_sa)[j];
                    }

                    if(m_has_shortcut[j]) {
                        m_shortcuts[m_rank(j)-1] = i;
                    }

                    i = j;
                }
            }
        });
    }

    // implements concept "DSProvider"
    template<dsid_t ds>
    inline void compress() {
        static_assert(ds == ds::INVERSE_SUFFIX_ARRAY, "ds not provided");
        // nothing to do, already sparse :-)
    }

    template<dsid_t ds>
    inline void discard() {
        static_assert(ds == ds::INVERSE_SUFFIX_ARRAY, "ds not provided");
        m_has_shortcut.clear();
        m_has_shortcut.shrink_to_fit();
        m_rank = Rank();
        m_shortcuts.clear();
        m_shortcuts.shrink_to_fit();
    }

    template<dsid_t ds>
    inline const tl::get<ds, ds_types>& get() {
        static_assert(ds == ds::INVERSE_SUFFIX_ARRAY, "ds not provided");
        return *this;
    }

    template<dsid_t ds>
    inline tl::get<ds, ds_types> relinquish() {
        static_assert(ds == ds::INVERSE_SUFFIX_ARRAY, "ds not provided");
        throw std::runtime_error("not supported"); //TODO: what to do?
    }

    // index access
    inline size_t operator[](size_t i) const {
        size_t j = i;
        bool s = true;

        while((*m_sa)[j] != i) {
            if(s && m_has_shortcut[j]) {
                j = m_shortcuts[m_rank(j)-1];
                s = false;
            } else {
                j = (*m_sa)[j];
            }
        }
        return j;
    }

    // size
    inline size_t size() const {
        return m_has_shortcut.size();
    }
};

} //ns
