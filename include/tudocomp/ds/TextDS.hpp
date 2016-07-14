#ifndef _INCLUDED_DS_TEXTDS_HPP
#define _INCLUDED_DS_TEXTDS_HPP

#include <sdsl/int_vector.hpp>

#include <tudocomp/ds/ITextDSProvider.hpp>

#include <tudocomp/ds/SuffixArray.hpp>
#include <tudocomp/ds/InverseSuffixArray.hpp>
#include <tudocomp/ds/PhiArray.hpp>
#include <tudocomp/ds/LCPArray.hpp>

#include <tudocomp/io.h>

namespace tudocomp {

using io::InputView;

/// Manages text related data structures.
class TextDS : public ITextDSProvider {

public:
    static const uint64_t SA = 0x01;
    static const uint64_t ISA = 0x02;
    static const uint64_t Phi = 0x04;
    static const uint64_t LCP = 0x08;

private:
    size_t m_size;
    const uint8_t* m_text;

    std::shared_ptr<SuffixArray>        m_sa;
    std::shared_ptr<InverseSuffixArray> m_isa;
    std::shared_ptr<PhiArray>           m_phi;
    std::shared_ptr<LCPArray>           m_lcp;

public:
    inline TextDS(const InputView& input)
        : m_size(input.size()), m_text((const uint8_t*)input.data())
    {
    }

    /// Constructs the required data structures as denoted by the given flags
    /// and releases all unwanted ones in a second step.
    inline void require(uint64_t flags) {
        //Step 1: construct
        if(flags & SA) require_sa(flags & ISA);
        if(flags & ISA) require_isa();
        if(flags & Phi) require_phi();
        if(flags & LCP) require_lcp(!(flags & Phi));

        //Step 2: release unwanted (may have been constructed beforehand)
        if(!(flags & SA)) release_sa();
        if(!(flags & ISA)) release_isa();
        if(!(flags & Phi)) release_phi();
        if(!(flags & LCP)) release_lcp();
    }

    /// Requires the Suffix Array to be constructed if not already present.
    virtual inline const SuffixArray& require_sa() override {
        return require_sa(false);
    }

    /// Requires the Suffix Array to be constructed if not already present.
    /// Optionally also constructs the Inverse Suffix Array just in time.
    inline const SuffixArray& require_sa(bool with_isa) {
        if(!m_sa) {
            m_sa = std::shared_ptr<SuffixArray>(new SuffixArray());

            if(with_isa && !m_isa) {
                m_isa = std::shared_ptr<InverseSuffixArray>(new InverseSuffixArray());
                m_sa->with_isa(
                    &*m_isa,
                    &InverseSuffixArray::construct_jit_init,
                    &InverseSuffixArray::construct_jit);
            }

            m_sa->construct(*this);
        }

        return *m_sa;
    }

    /// Requires the Phi Array to be constructed if not already present.
    virtual inline const PhiArray& require_phi() override {
        if(!m_phi) {
            m_phi = std::shared_ptr<PhiArray>(new PhiArray());
            m_phi->construct(*this);
        }

        return *m_phi;
    }

    /// Requires the Inverse Suffix Array to be constructed if not already present.
    virtual inline const InverseSuffixArray& require_isa() override {
        if(!m_isa) {
            m_isa = std::shared_ptr<InverseSuffixArray>(new InverseSuffixArray());
            m_isa->construct(*this);
        }

        return *m_isa;
    }

    /// Requires the LCP Array to be constructed if not already present.
    virtual inline const LCPArray& require_lcp() override {
        return require_lcp(false);
    }
    
    /// Requires the Suffix Array to be constructed if not already present.
    /// Optionally consumes the Phi array for in-place construction.
    inline const LCPArray& require_lcp(bool consume_phi) {
        if(!m_lcp) {
            m_lcp = std::shared_ptr<LCPArray>(new LCPArray());
            m_lcp->construct(*this, consume_phi);
        }

        return *m_lcp;
    }

    /// Releases the suffix array if present.
    virtual inline void release_sa() override {
        m_sa.reset();
    }

    /// Releases the inverse suffix array array if present.
    virtual inline void release_isa() override {
        m_isa.reset();
    }

    /// Releases the Phi array if present.
    virtual inline void release_phi() override {
        m_phi.reset();
    }

    /// Releases the LCP array if present.
    virtual inline void release_lcp() override {
        m_lcp.reset();
    }

    /// Accesses the input text at position i.
    virtual inline uint8_t operator[](size_t i) const override {
        return m_text[i];
    }

    /// Provides access to the input text.
    virtual inline const uint8_t* text() const override {
        return m_text;
    }

    /// Returns the size of the input text.
    virtual inline size_t size() const override {
        return m_size;
    }
};

}

#endif

