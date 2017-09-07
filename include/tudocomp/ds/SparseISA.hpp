#pragma once

#include <assert.h>
#include <type_traits>

#include <tudocomp/ds/TextDSFlags.hpp>
#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/rank/Rank.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {

/// Constructs the inverse suffix array using the suffix array.
template<typename sa_t>
class SparseISA : public Algorithm {
public:
    using iv_t = DynamicIntVector;
    using data_type = iv_t;

private:
    const sa_t* m_sa;

    BitVector m_has_shortcut;
    Rank      m_rank;

    iv_t m_shortcuts;

public:
    inline static Meta meta() {
        Meta m("isa", "sparse_isa");
        m.option("sa").templated<sa_t>("sa");
        m.option("t").dynamic(3);
        return m;
    }

    inline static ds::InputRestrictions restrictions() {
        return ds::InputRestrictions {};
    }

    template<typename textds_t>
    inline SparseISA(Env&& env, textds_t& tds, CompressMode cm)
            : Algorithm(std::move(env)) {

        // Suffix Array types must match
        static_assert(std::is_same<sa_t, typename textds_t::sa_type>(),
            "Suffix Array type mismatch!");

        // Require Suffix Array
        m_sa = &tds.require_sa(cm);

        const size_t n = m_sa->size();
        m_has_shortcut = BitVector(n);

        const size_t t = this->env().option("t").as_integer();

        // Construct
        {
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
        }
    }

public:
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

    inline void compress() {
        // nothing to do, already sparse :-)
    }

    inline size_t size() const {
        return m_has_shortcut.size();
    }

    /// \brief Forces the data structure to relinquish its data storage.
    ///
    /// This is done by moving the ownership of the storage to the caller.
    /// After this operation, the data structure will behave as if it was
    /// empty, and may throw debug assertions on access.
    inline iv_t relinquish() {
        throw std::runtime_error("not supported"); //TODO: what to do?
    }

    /// \brief Creates a copy of the data structure's storage.
    inline iv_t copy() const {
        throw std::runtime_error("not supported"); //TODO: what to do?
    }
};

} //ns
