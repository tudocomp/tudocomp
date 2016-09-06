#ifndef _INCLUDED_DS_INVERSE_SUFFIX_ARRAY_HPP
#define _INCLUDED_DS_INVERSE_SUFFIX_ARRAY_HPP

#include <tudocomp/util.h>
#include <tudocomp/ds/ITextDSProvider.hpp>
#include <tudocomp/ds/SuffixArray.hpp>

namespace tudocomp {

using io::InputView;

class InverseSuffixArray {

public:
    typedef sdsl::int_vector<> iv_t;

private:
    iv_t m_isa;

public:
    inline InverseSuffixArray() {
    }

    inline iv_t& data() {
        return m_isa;
    }

    inline const iv_t& data() const {
        return m_isa;
    }

    inline iv_t::value_type operator[](iv_t::size_type i) const {
        return m_isa[i];
    }

    inline iv_t::size_type size() const {
        return m_isa.size();
    }

    inline void construct(ITextDSProvider& t) {
        auto& sa = t.require_sa();
        auto n = sa.size();

        m_isa = iv_t(n, 0, bits_for(n));
        for(size_t i = 0; i < n; i++) {
            m_isa[sa[i]] = i;
        }
    }

    // Initializing callback for just-in-time construction
    static inline void construct_jit_init(InverseSuffixArray& isa, size_t n) {
        isa.m_isa = iv_t(n, 0, bits_for(n));
    }

    // Just-in-time construction callback
    static inline void construct_jit(InverseSuffixArray& isa, size_t i, size_t v) {
        isa.m_isa[v] = i;
    }
};

}

#endif

