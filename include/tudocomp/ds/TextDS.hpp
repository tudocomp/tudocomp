#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/ds/IntVector.hpp>

//Defaults
#include <tudocomp/ds/SADivSufSort.hpp>
#include <tudocomp/ds/ISASimple.hpp>
#include <tudocomp/ds/LCPPhi.hpp>

namespace tdc {

static_assert(
    std::is_same<View::value_type, uliteral_t>::value,
    "View::value_type and uliteral_t must be the same");

/// Manages text related data structures.
template<
    typename sa_t = SADivSufSort,
    typename isa_t = ISASimple,
    typename lcp_t = LCPPhi>
class TextDS : public Algorithm {
public:
    using dsflags_t = unsigned int;
    static const dsflags_t SA  = 0x01;
    static const dsflags_t ISA = 0x02;
    static const dsflags_t LCP = 0x04;

    using value_type = uliteral_t;
    using sa_type = sa_t;
    using isa_type = isa_t;
    using lcp_type = lcp_t;

private:
    View m_text;

    std::unique_ptr<sa_t>  m_sa;
    std::unique_ptr<isa_t> m_isa;
    std::unique_ptr<lcp_t> m_lcp;

    template<typename ds_t>
    inline std::unique_ptr<ds_t> construct_ds(const std::string& option) {
        std::unique_ptr<ds_t> ds = std::make_unique<ds_t>(
            env().env_for_option(option));

        ds->construct(*this);
        return ds;
    }

public:
    inline static Meta meta() {
        Meta m("textds", "textds");
        m.option("sa").templated<sa_t, SADivSufSort>();
        m.option("isa").templated<isa_t, ISASimple>();
        m.option("lcp").templated<lcp_t, LCPPhi>();
        return m;
    }

    inline TextDS(Env&& env, const View& text)
        : Algorithm(std::move(env)),
          m_text(text) {

        if(!m_text.ends_with(uint8_t(0))){
             throw std::logic_error(
                 "Input has no sentinel! Please make sure you declare "
                 "the compressor calling this with "
                 "`m.needs_sentinel_terminator()` in its `meta()` function."
            );
        }
    }

    inline TextDS(Env&& env, const View& text, dsflags_t flags)
        : TextDS(std::move(env), text) {

        require(flags);
    }

    inline const sa_t& require_sa() {
        if(!m_sa) m_sa = std::move(construct_ds<sa_t>("sa"));
        return *m_sa;
    }
    inline std::unique_ptr<sa_t> release_sa() { return std::move(m_sa); }

    inline const isa_t& require_isa() {
        if(!m_isa) m_isa = std::move(construct_ds<isa_t>("isa"));
        return *m_isa;
    }
    inline std::unique_ptr<isa_t> release_isa() { return std::move(m_isa); }

    inline const lcp_t& require_lcp() {
        if(!m_lcp) m_lcp = std::move(construct_ds<lcp_t>("lcp"));
        return *m_lcp;
    }
    inline std::unique_ptr<lcp_t> release_lcp() { return std::move(m_lcp); }

    inline void require(dsflags_t flags) {
        // construct requested structures
        if(flags & SA) require_sa();
        if(flags & LCP) require_lcp();
        if(flags & ISA) require_isa();

        // release unrequested structures
        if(!(flags & SA)) release_sa();
        if(!(flags & LCP)) release_lcp();
        if(!(flags & ISA)) release_isa();
    }

    /// Accesses the input text at position i.
    inline value_type operator[](size_t i) const {
        return m_text[i];
    }

    /// Provides direct access to the input text.
    inline const value_type* text() const {
        return m_text.data();
    }

    /// Returns the size of the input text.
    inline size_t size() const {
        return m_text.size();
    }

    inline void print(std::ostream& out, size_t base) {
        size_t w = std::max(6UL, (size_t)std::log10((double)size()) + 1);
        out << std::setfill(' ');

        //Heading
        out << std::setw(w) << "i" << " | ";
        if(m_sa) out << std::setw(w) << "SA[i]" << " | ";
        if(m_lcp) out << std::setw(w) << "LCP[i]" << " | ";
        if(m_isa) out << std::setw(w) << "ISA[i]" << " | ";
        out << std::endl;

        //Separator
        out << std::setfill('-');
        out << std::setw(w) << "" << "-|-";
        if(m_sa) out << std::setw(w) << "" << "-|-";
        if(m_lcp) out << std::setw(w) << "" << "-|-";
        if(m_isa) out << std::setw(w) << "" << "-|-";
        out << std::endl;

        //Body
        out << std::setfill(' ');
        for(size_t i = 0; i < size(); i++) {
            out << std::setw(w) << (i + base) << " | ";
            if(m_sa) out << std::setw(w) << ((*m_sa)[i] + base) << " | ";
            if(m_lcp) out << std::setw(w) << (*m_lcp)[i] << " | ";
            if(m_isa) out << std::setw(w) << ((*m_isa)[i] + base) << " | ";
            out << std::endl;
        }
    }
};

} //ns
