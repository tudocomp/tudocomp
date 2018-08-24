#pragma once

#include <tudocomp/util.hpp>

#include <tudocomp/Compressor.hpp>

#include <tudocomp/compressors/lzss/FactorBuffer.hpp>
#include <tudocomp/compressors/lzss/FactorizationStats.hpp>
#include <tudocomp/compressors/lzss/UnreplacedLiterals.hpp>

#include <tudocomp/ds/TextDS.hpp>

#include <tudocomp_stat/StatPhase.hpp>

// For default params
#include <tudocomp/compressors/lcpcomp/decompress/ScanDec.hpp>
#include <tudocomp/compressors/lcpcomp/compress/ArraysComp.hpp>

namespace tdc {

/// Factorizes the input by finding redundant phrases in a re-ordered version
/// of the LCP table.
template<typename lzss_coder_t, typename strategy_t, typename dec_t, typename text_t = TextDS<>>
class LCPCompressor : public Compressor {
public:
    inline static Meta meta() {
        Meta m("compressor", "lcpcomp");
        m.option("coder").templated<lzss_coder_t>("lzss_coder");
        m.option("comp").templated<strategy_t, lcpcomp::ArraysComp>("lcpcomp_comp");
        m.option("dec").templated<dec_t, lcpcomp::ScanDec>("lcpcomp_dec");
        m.option("textds").templated<text_t, TextDS<>>("textds");
        m.option("threshold").dynamic(5);
        m.option("flatten").dynamic(1); // 0 or 1
        m.uses_textds<text_t>(strategy_t::textds_flags());
        return m;
    }

    /// Construct the class with an environment.
    inline LCPCompressor(Env&& env) : Compressor(std::move(env)) {}

    inline virtual void compress(Input& input, Output& output) override {
        auto in = input.as_view();
        DCHECK(in.ends_with(uint8_t(0)));

        auto text = StatPhase::wrap("Construct Text DS", [&]{
            return text_t(env().env_for_option("textds"), in, strategy_t::textds_flags());
        });

        // read options
        const len_t threshold = env().option("threshold").as_integer(); //factor threshold
        lzss::FactorBufferRAM factors;

        StatPhase::wrap("Factorize", [&]{
            // Factorize
            strategy_t strategy(env().env_for_option("comp"));
            strategy.factorize(text, threshold, factors);
        });

        // sort factors
        StatPhase::wrap("Sort Factors", [&]{ factors.sort(); });

        if(env().option("flatten").as_integer()) {
            // flatten factors
            StatPhase::wrap("Flatten Factors", [&]{ factors.flatten(); });
        }

        // statistics
        IF_STATS({
            lzss::FactorizationStats stats(factors, text.size());
            stats.log();
        })

        // encode
        StatPhase::wrap("Encode", [&]{
            auto coder = lzss_coder_t(env().env_for_option("coder")).encoder(
                output, lzss::UnreplacedLiterals<text_t,decltype(factors)>(text, factors));

            coder.encode_text(text, factors);
        });
    }

    inline virtual void decompress(Input& input, Output& output) override {
        dec_t decomp(env().env_for_option("dec"));

        {
            auto decoder = lzss_coder_t(env().env_for_option("coder")).decoder(input);
            decoder.decode(decomp);
        }

        auto outs = output.as_stream();
        decomp.write_to(outs);
    }
};

/// \brief Contains factorization and decoding strategies for
///        the  \ref LCPCompressor.
namespace lcpcomp {
}

}

