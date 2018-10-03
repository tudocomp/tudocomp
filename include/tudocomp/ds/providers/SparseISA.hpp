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
class SparseISA : public Algorithm {
private:
    using sa_t = typename sa_provider_t::sa_t;

public:
    inline static Meta meta() {
        Meta m(ds::provider_type(), "sparse_isa");
        m.param("sa").strategy<sa_provider_t>(ds::provider_type());
        m.param("t").primitive(3);
        return m;
    }

    class Data {
        friend class SparseISA;

    private:
        const sa_t* m_sa;

        BitVector m_has_shortcut;
        Rank      m_rank;

        DynamicIntVector m_shortcuts;

        inline Data() : m_sa(nullptr) {
        }

    public:
        inline Data(Data&& other)
            : m_sa(other.m_sa),
              m_has_shortcut(std::move(other.m_has_shortcut)),
              m_rank(std::move(other.m_rank)),
              m_shortcuts(std::move(other.m_shortcuts))
        {
        }

        inline Data(const Data& other)
            : m_sa(other.m_sa),
              m_has_shortcut(other.m_has_shortcut),
              m_rank(other.m_rank),
              m_shortcuts(other.m_shortcuts)
        {
        }

        inline Data& operator=(Data&& other) {
            m_sa = other.m_sa;
            m_has_shortcut = std::move(other.m_has_shortcut);
            m_rank         = std::move(other.m_rank);
            m_shortcuts    = std::move(other.m_shortcuts);
            return *this;
        }

        inline Data& operator=(const Data& other) {
            m_sa = other.m_sa;
            m_has_shortcut = other.m_has_shortcut;
            m_rank         = other.m_rank;
            m_shortcuts    = other.m_shortcuts;
            return *this;
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

private:
    Data m_data;

public:
    using Algorithm::Algorithm;

    using provides = std::index_sequence<ds::INVERSE_SUFFIX_ARRAY>;
    using requires = std::index_sequence<ds::SUFFIX_ARRAY>;
    using ds_types = tl::set<ds::INVERSE_SUFFIX_ARRAY, Data>;

    // implements concept "DSProvider"
    template<typename manager_t>
    inline void construct(manager_t& manager, bool compressed_space) {
        // Require Suffix Array
        auto& sa = manager.template get<ds::SUFFIX_ARRAY>();
        m_data.m_sa = &sa;

        const size_t n = sa.size();
        m_data.m_has_shortcut = BitVector(n);

        const size_t t = this->config().param("t").as_uint();

        // Construct
        StatPhase::wrap("Construct sparse ISA", [&]{
            auto v = BitVector(n);
            for(size_t i = 0; i < n; i++) {
                if(!v[i]) {
                    // new cycle
                    v[i] = 1;
                    size_t j = sa[i];
                    size_t k = 1;

                    while(j != i) {
                        if((k % t) == 0) {
                            m_data.m_has_shortcut[j] = 1;
                        }

                        v[j] = 1;
                        j = sa[j];
                        ++k;
                    }

                    if(k > t) m_data.m_has_shortcut[i] = 1;
                }
            }

            m_data.m_rank = Rank(m_data.m_has_shortcut);
            m_data.m_shortcuts = DynamicIntVector(
                m_data.m_rank(n-1), 0, bits_for(n));

            for(size_t i = 0; i < n; i++) {
                if(v[i]) {
                    v[i] = 0;
                    size_t j = sa[i];
                    while(v[j]) {
                        if(m_data.m_has_shortcut[j]) {
                            m_data.m_shortcuts[m_data.m_rank(j)-1] = i;
                            i = j;
                        }
                        v[j] = 0;
                        j = sa[j];
                    }

                    if(m_data.m_has_shortcut[j]) {
                        m_data.m_shortcuts[m_data.m_rank(j)-1] = i;
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
        m_data = Data();
    }

    template<dsid_t ds>
    inline const tl::get<ds, ds_types>& get() {
        static_assert(ds == ds::INVERSE_SUFFIX_ARRAY, "ds not provided");
        return m_data;
    }

    template<dsid_t ds>
    inline tl::get<ds, ds_types> relinquish() {
        static_assert(ds == ds::INVERSE_SUFFIX_ARRAY, "ds not provided");
        return std::move(m_data);
    }
};

} //ns
