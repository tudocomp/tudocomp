#ifndef _INCLUDED_ESA_COMPRESSOR_HPP_
#define _INCLUDED_ESA_COMPRESSOR_HPP_

#include <tudocomp/util.hpp>

#include <tudocomp/compressors/lzss/LZSSCoding.hpp>
#include <tudocomp/compressors/lzss/LZSSFactors.hpp>
#include <tudocomp/compressors/lzss/LZSSLiterals.hpp>

#include <tudocomp/compressors/esacomp/ESACompMaxLCP.hpp>
#include <tudocomp/compressors/esacomp/ESACompBulldozer.hpp>
#include <tudocomp/compressors/esacomp/ESACompNaive.hpp>

#include <tudocomp/ds/TextDS.hpp>

namespace tdc {

/// Factorizes the input by finding redundant phrases in a re-ordered version
/// of the LCP table.
template<typename strategy_t, typename coder_t>
class ESACompressor : public Compressor {
private:
    typedef TextDS<> text_t;

public:
    inline static Meta meta() {
        Meta m("compressor", "esacomp");
        m.option("coder").templated<coder_t>();
        m.option("strategy").templated<strategy_t, esacomp::ESACompMaxLCP>();
        m.option("threshold").dynamic("6");
        return m;
    }

    /// Construct the class with an environment.
    inline ESACompressor(Env&& env) : Compressor(std::move(env)) {}

    inline virtual void compress(Input& input, Output& output) override {
        auto in = input.as_view();
		in.ensure_null_terminator();
        text_t text(in);

        // read options
        size_t threshold = env().option("threshold").as_integer(); //factor threshold
        lzss::FactorBuffer factors;

        {
            // Construct SA, ISA and LCP
            env().begin_stat_phase("Construct text ds");
            text.require(text_t::SA | text_t::ISA | text_t::LCP);
            env().end_stat_phase();

            // Factorize
            env().begin_stat_phase("Factorize using strategy");

            strategy_t strategy(env().env_for_option("strategy"));
            strategy.factorize(text, threshold, factors);

            env().log_stat("threshold", threshold);
            env().log_stat("factors", factors.size());
            env().end_stat_phase();
        }

        // sort factors
        factors.sort();

        // encode
        typename coder_t::Encoder coder(env().env_for_option("coder"),
            output, lzss::TextLiterals<text_t>(text, factors));

        lzss::encode_text(coder, text, factors,
            in.is_terminal_null_ensured()); //TODO is this correct?
    }

    inline virtual void decompress(Input& input, Output& output) override {
        //TODO: tell that forward-factors are allowed
        typename coder_t::Decoder decoder(env().env_for_option("coder"), input);
        auto outs = output.as_stream();

        lzss::decode_text(decoder, outs, true);
    }
};

}

#endif
