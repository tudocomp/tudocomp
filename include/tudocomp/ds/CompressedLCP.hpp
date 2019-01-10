#pragma once

#include <type_traits>

#include <tudocomp/ds/TextDSFlags.hpp>
#include <tudocomp/ds/CompressMode.hpp>
#include <tudocomp/ds/Select.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {

/// \brief Constructs the LCP array from the Suffix and PLCP arrays,
///        storing it in a manner as described by Fischer (WeeLCP, 2010).
template<typename sa_t>
class CompressedLCP : public Algorithm {
public:
    using iv_t = DynamicIntVector;
    using data_type = iv_t;

private:
    const sa_t* m_sa;

    len_t     m_size;
    len_t     m_max;

    BitVector m_lcp;
    Select1   m_select;

public:
    inline static Meta meta() {
        Meta m(TypeDesc("lcp"), "compressed_lcp",
            "Constructs the LCP array from the Suffix and PLCP arrays, "
            "storing it in the \"WeeLCP\" [Fischer, 2010] manner.");
        m.param("sa", "The suffix array implementation.")
            .strategy<sa_t>(TypeDesc("sa"));
        return m;
    }

private:
    inline void encode_unary(len_t x) {
        while(x) {
            --x;
            m_lcp.emplace_back(0);
        }
        m_lcp.emplace_back(1);
    }

public:
    template<typename textds_t>
    inline CompressedLCP(Config&& cfg, textds_t& tds, CompressMode cm)
            : Algorithm(std::move(cfg)) {

        // Suffix Array types must match
        static_assert(std::is_same<sa_t, typename textds_t::sa_type>(),
            "Suffix Array type mismatch!");

        // Require Suffix and PLCP Array
        m_sa = &tds.require_sa(cm);
        auto& plcp = tds.require_plcp(cm);

        m_size = plcp.size();
        m_max  = plcp.max_lcp();

        // Construct
        StatPhase::wrap("Construct compressed LCP Array", [&]{
            encode_unary(plcp[0] + 1);
            for(size_t i = 1; i < m_size; i++) {
                encode_unary(plcp[i] - plcp[i-1] + 1);
            }

            m_select = Select1(m_lcp);
        });
    }

	inline len_t max_lcp() const {
		return m_max;
	}

    inline len_t operator[](len_t i) const {
        const len_t j = (*m_sa)[i];
        return m_select(j+1) - 2*j - 1;
    }

    inline void compress() {
        // nothing to do, already succinct :-)
    }

    inline size_t size() const {
        return m_size;
    }

    inline iv_t relinquish() {
        throw std::runtime_error("not supported"); //TODO: what to do?
    }

    inline iv_t copy() const {
        throw std::runtime_error("not supported"); //TODO: what to do?
    }
};

}
