#pragma once

#include <cmath>
#include <ostream>

#include <sdsl/int_vector.hpp>

#include <tudocomp/io.hpp>
#include "forward.hpp"

#include <tudocomp/Literal.hpp>
#include <tudocomp/Env.hpp>

namespace tdc {

/// Manages text related data structures.
template<
    class lcp_tt = LCPArray,
    template<typename> class isa_tt = InverseSuffixArray>
class TextDS {
public:
    typedef uint8_t value_type;
    static const uint64_t SA = 0x01;
    static const uint64_t ISA = 0x02;
    static const uint64_t Phi = 0x04;
    static const uint64_t LCP = 0x08;
    typedef TextDS<lcp_tt,isa_tt> this_t;
    typedef typename lcp_tt::template construct_t<this_t> lcp_t;
    typedef isa_tt<this_t> isa_t;
    typedef SuffixArray<this_t> sa_t;
    typedef PhiArray<this_t> phi_t;

private:
    class OptionalEnv {
        Env* m_env;
    public:
        inline OptionalEnv(Env* env): m_env(env) {}
        inline void begin_stat_phase(string_ref name) {
            if (m_env != nullptr) {
                m_env->begin_stat_phase(name);
            }
        }
        inline void end_stat_phase() {
            if (m_env != nullptr) {
                m_env->end_stat_phase();
            }
        }
    };

    View m_text;
    OptionalEnv m_env;

    std::unique_ptr<sa_t>  m_sa;
    std::unique_ptr<isa_t> m_isa;
    std::unique_ptr<phi_t> m_phi;
    std::unique_ptr<lcp_t> m_lcp;

    class Literals {
    protected:
        const this_t* m_text;
        size_t m_pos;

    public:
        inline Literals(const this_t& text) : m_text(&text), m_pos(0) {}

        inline bool has_next() const {
            return m_pos < m_text->size();
        }

        inline Literal next() {
            assert(has_next());

            auto i = m_pos++;
            return {(*m_text)[i], i};
        }
    };
private:
    inline static void null_check(View view) {
        if(!view.ends_with(uint8_t(0))){
             throw std::logic_error(
                 "Input is not 0 terminated! Please make sure you declare "
                 "the compressor calling this with "
                 "`m.needs_sentinel_terminator()` in its `meta()` function."
            );
        }
    }

public:
    OptionalEnv env() {
		return m_env;
	}

    inline TextDS(const View& input) : m_text(input), m_env(nullptr) {
        null_check(input);
    }

    inline TextDS(const View& input, uint64_t flags): TextDS(input) {
        null_check(input);
        require(flags);
    }

    inline TextDS(const View& input, Env& env): m_text(input), m_env(&env) {
        null_check(input);
    }

    inline TextDS(const View& input, Env& env, uint64_t flags): TextDS(input, env) {
        null_check(input);
        require(flags);
    }

    /// Constructs the required data structures as denoted by the given flags
    /// and releases all unwanted ones in a second step.
    inline void require(uint64_t flags) {
        //Step 1: construct
        if(flags & SA) require_sa();
        if(flags & ISA) require_isa();
        if(flags & Phi) require_phi();
        if(flags & LCP) require_lcp();

        //Step 2: release unwanted (may have been constructed beforehand)
        if(!(flags & SA)) release_sa();
        if(!(flags & ISA)) release_isa();
        if(!(flags & Phi)) release_phi();
        if(!(flags & LCP)) release_lcp();
    }

    /// Requires the Suffix Array to be constructed if not already present.
    inline const sa_t& require_sa();

    /// Requires the Phi Array to be constructed if not already present.
    inline const phi_t& require_phi();

    /// Requires the Inverse Suffix Array to be constructed if not already present.
    inline const isa_t& require_isa();

    /// Requires the LCP Array to be constructed if not already present.
    inline const lcp_t& require_lcp();


    /// Releases the suffix array if present.
    inline std::unique_ptr<sa_t> release_sa() {
        return std::move(m_sa);
    }

    /// Releases the inverse suffix array array if present.
    inline std::unique_ptr<isa_t> release_isa() {
        return std::move(m_isa);
    }

    /// Releases the Phi array if present.
    inline std::unique_ptr<phi_t> release_phi() {
        return std::move(m_phi);
    }

    /// Releases the LCP array if present.
    inline std::unique_ptr<lcp_t> release_lcp() {
        return std::move(m_lcp);
    }

    /// Accesses the input text at position i.
    inline value_type operator[](size_t i) const {
        return m_text[i];
    }

    /// Provides access to the input text.
    inline const value_type* text() const {
        return m_text.data();
    }

    /// Returns the size of the input text.
    inline size_t size() const {
        return m_text.size();
    }

    /// Prints the constructed tables.
    inline void print(std::ostream& out, size_t base = 0);

    inline Literals literals() const {
        return iterator(*this, 0);
    }
};

}//ns

#include <tudocomp/ds/SuffixArray.hpp>
#include <tudocomp/ds/InverseSuffixArray.hpp>
#include <tudocomp/ds/PhiArray.hpp>
#include <tudocomp/ds/LCPArray.hpp>

namespace tdc {

template<
    class lcp_tt,
    template<typename> class isa_tt>
inline void TextDS<lcp_tt,isa_tt>::print(std::ostream& out, size_t base) {
    size_t w = std::max(6UL, (size_t)std::log10((double)size()) + 1);
    out << std::setfill(' ');

    //Heading
    out << std::setw(w) << "i" << " | ";
    if(m_sa) out << std::setw(w) << "SA[i]" << " | ";
    if(m_isa) out << std::setw(w) << "ISA[i]" << " | ";
    if(m_phi) out << std::setw(w) << "Phi[i]" << " | ";
    if(m_lcp) out << std::setw(w) << "LCP[i]" << " | ";
    out << std::endl;

    //Separator
    out << std::setfill('-');
    out << std::setw(w) << "" << "-|-";
    if(m_sa) out << std::setw(w) << "" << "-|-";
    if(m_isa) out << std::setw(w) << "" << "-|-";
    if(m_phi) out << std::setw(w) << "" << "-|-";
    if(m_lcp) out << std::setw(w) << "" << "-|-";
    out << std::endl;

    //Body
    out << std::setfill(' ');
    for(size_t i = 0; i < size() + 1; i++) {
        out << std::setw(w) << (i + base) << " | ";
        if(m_sa) out << std::setw(w) << ((*m_sa)[i] + base) << " | ";
        if(m_isa) out << std::setw(w) << ((*m_isa)[i] + base) << " | ";
        if(m_phi) out << std::setw(w) << ((*m_phi)[i] + base) << " | ";
        if(m_lcp) out << std::setw(w) << (*m_lcp)[i] << " | ";
        out << std::endl;
    }
}

template<
    class lcp_tt,
    template<typename> class isa_tt>
const SuffixArray<TextDS<lcp_tt,isa_tt>>& TextDS<lcp_tt,isa_tt>::require_sa() {
    if(!m_sa) {
        m_env.begin_stat_phase("construct suffix array");
        m_sa = std::make_unique<sa_t>();
        m_sa->construct(*this);
        m_env.end_stat_phase();
    }
    return *m_sa;
}

template<
    class lcp_tt,
    template<typename> class isa_tt>
const PhiArray<TextDS<lcp_tt,isa_tt>>& TextDS<lcp_tt,isa_tt>::require_phi() {
    if(!m_phi) {
        m_env.begin_stat_phase("construct phi array");
        m_phi = std::make_unique<phi_t>();
        m_phi->construct(*this);
        m_env.end_stat_phase();
    }

    return *m_phi;
}

template<
    class lcp_tt,
    template<typename> class isa_tt>
const isa_tt<TextDS<lcp_tt,isa_tt>>& TextDS<lcp_tt,isa_tt>::require_isa() {
    if(!m_isa) {
        m_env.begin_stat_phase("construct inverse suffix array");
        m_isa = std::make_unique<isa_t>();
        m_isa->construct(*this);
        m_env.end_stat_phase();
    }

    return *m_isa;
}

template<class lcp_tt,
    template<typename> class isa_tt>
const typename TextDS<lcp_tt,isa_tt>::lcp_t& TextDS<lcp_tt,isa_tt>::require_lcp() {
    if(!m_lcp) {
        m_env.begin_stat_phase("construct lcp array");
        m_lcp = std::make_unique<lcp_t>();
        m_lcp->construct(*this);
        m_env.end_stat_phase();
    }
    return *m_lcp;
}

//typedef TextDS<lcp_sada<TextDS<lcp_sada,InverseSuffixArray>, InverseSuffixArray<TextDS<lcp_sada,InverseSuffixArray>> TextDSI;
typedef TextDS<LCPArray, InverseSuffixArray> TextDSI;
}//ns

