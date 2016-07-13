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

    /// Requires the Suffix Array to be constructed if not already present.
    virtual inline const SuffixArray& require_sa() override {
        if(!m_sa) {
            m_sa = std::shared_ptr<SuffixArray>(new SuffixArray());
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
        if(!m_lcp) {
            m_lcp = std::shared_ptr<LCPArray>(new LCPArray());
            m_lcp->construct(*this);
        }

        return *m_lcp;
    }

    virtual inline uint8_t operator[](size_t i) const override {
        return m_text[i];
    }

    virtual inline const uint8_t* text() const override {
        return m_text;
    }

    virtual inline size_t size() const override {
        return m_size;
    }
};

}

#endif

