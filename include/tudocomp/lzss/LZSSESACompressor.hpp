#ifndef _INCLUDED_LZSS_ESA_COMPRESSOR_HPP_
#define _INCLUDED_LZSS_ESA_COMPRESSOR_HPP_

#include <tudocomp/ds/SuffixArray.hpp>
#include <tudocomp/ds/LCPArray.hpp>

#include <tudocomp/sdslex/int_vector_wrapper.hpp>
#include <tudocomp/util.h>

#include <tudocomp/lzss/LZSSCompressor.hpp>

#include <tudocomp/lzss/esacomp/ESACompBulldozer.hpp>
#include <tudocomp/lzss/esacomp/ESACompCollider.hpp>
#include <tudocomp/lzss/esacomp/ESACompMaxLCP.hpp>
namespace tudocomp {
namespace lzss {

/// Factorizes the input by finding redundant phrases in a re-ordered version
/// of the LCP table.
template<typename S, typename C>
class LZSSESACompressor : public LZSSCompressor<C> {

public:
    inline static Meta meta() {
        Meta m("compressor", "esacomp");
        m.option("coder").templated<C>();
        m.option("strategy").templated<S, ESACompMaxLCP>();
        return m;
    }

    /// Default constructor (not supported).
    inline LZSSESACompressor() = delete;

    /// Construct the class with an environment.
    inline LZSSESACompressor(Env&& env) :
        LZSSCompressor<C>(std::move(env)) {}

    /// \copydoc
    inline virtual bool pre_factorize(Input& input) override {
        auto& env = this->env();
        auto in = input.as_view();

        size_t len = in.size();
        const uint8_t* in_ptr = (const uint8_t*)in.data();
        sdslex::int_vector_wrapper wrapper(in_ptr, len);

        //Construct SA
        env.begin_stat_phase("Construct SA and ISA");
        SuffixArray sa(in);
        env.end_stat_phase();

        //Construct LCP
        env.begin_stat_phase("Construct LCP");
        LCPArray lcp(in, sa);
        env.end_stat_phase();

        //Use strategy to generate factors
        size_t fact_min = 3; //factor threshold
        std::vector<LZSSFactor>& factors = LZSSCompressor<C>::m_factors;

        env.begin_stat_phase("Factorize using strategy");

        S interval_selector(env.env_for_option("strategy"));
        interval_selector.factorize(sa, lcp, fact_min, factors);

        env.log_stat("threshold", fact_min);
        env.log_stat("factors", factors.size());
        env.end_stat_phase();

        //sort
        env.begin_stat_phase("Sort factors");
        std::sort(factors.begin(), factors.end());
        env.end_stat_phase();

        return true;
    }

    /// \copydoc
    inline virtual LZSSCoderOpts coder_opts(Input& input) override {
        return LZSSCoderOpts(false, bitsFor(input.size()));
    }

    /// \copydoc
    inline virtual void factorize(Input& input) override {
    }

    virtual Env create_decoder_env() override {
        return this->env().env_for_option("coder");
    }
};

}}

#endif
