#pragma once

#include <tudocomp/util.hpp>

#include <tudocomp/Compressor.hpp>
#include <tudocomp/compressors/lzss/LZSSCoding.hpp>
#include <tudocomp/compressors/lzss/LZSSFactors.hpp>
#include <tudocomp/compressors/lzss/LZSSLiterals.hpp>

#include <tudocomp/ds/TextDS.hpp>


namespace tdc {
namespace lcpcomp {
class MaxLCPStrategy;
class CompactDec;

template<typename coder_t, typename decode_buffer_t>
inline void decode_text_internal(Env&& env, coder_t& decoder, std::ostream& outs) {

    decoder.env().begin_stat_phase("Starting Decoding");
    // decode text range
    auto text_len = decoder.template decode<len_t>(len_r);
    Range text_r(text_len);

    // decode shortest and longest factor
    auto flen_min = decoder.template decode<len_t>(text_r);
    auto flen_max = decoder.template decode<len_t>(text_r);
    MinDistributedRange flen_r(flen_min, flen_max);

    // decode longest distance between factors
    auto fdist_max = decoder.template decode<len_t>(text_r);
    Range fdist_r(fdist_max);

    // init decode buffer
//    decode_buffer_t buffer(text_len);
    decode_buffer_t  buffer(std::move(env), text_len);

    // decode
    while(!decoder.eof()) {
        len_t num;

        auto b = decoder.template decode<bool>(bit_r);
        if(b) num = decoder.template decode<len_t>(fdist_r);
        else  num = 0;

        // decode characters
        while(num--) {
            auto c = decoder.template decode<uliteral_t>(literal_r);
            buffer.decode_literal(c);
        }

        if(!decoder.eof()) {
            //decode factor
            auto src = decoder.template decode<len_t>(text_r);
            auto len = decoder.template decode<len_t>(flen_r);

            buffer.decode_factor(src, len);
        }
    }
   decoder.env().end_stat_phase();
   decoder.env().begin_stat_phase("Scan Decoding");
    buffer.decode_lazy();
   decoder.env().end_stat_phase();
   decoder.env().begin_stat_phase("Eager Decoding");
    buffer.decode_eagerly();
    IF_STATS(decoder.env().log_stat("longest_chain", buffer.longest_chain()));
   decoder.env().end_stat_phase();
   decoder.env().begin_stat_phase("Output Text");
    buffer.write_to(outs);
   decoder.env().end_stat_phase();
   decoder.env().end_stat_phase();
}

}//ns

/// Factorizes the input by finding redundant phrases in a re-ordered version
/// of the LCP table.
template<typename coder_t, typename strategy_t, typename dec_t, typename text_t = TextDS<>>
class LCPCompressor : public Compressor {
public:
    inline static Meta meta() {
        Meta m("compressor", "lcpcomp");
        m.option("coder").templated<coder_t>();
        m.option("comp").templated<strategy_t, lcpcomp::MaxLCPStrategy>();
        m.option("dec").templated<dec_t, lcpcomp::CompactDec>();
        m.option("textds").templated<text_t, TextDS<>>();
        m.option("threshold").dynamic(3);
        m.uses_textds<text_t>(strategy_t::textds_flags());
        return m;
    }

    /// Construct the class with an environment.
    inline LCPCompressor(Env&& env) : Compressor(std::move(env)) {}

    inline virtual void compress(Input& input, Output& output) override {
        auto in = input.as_view();
        DCHECK(in.ends_with(uint8_t(0)));
        text_t text(env().env_for_option("textds"), in, strategy_t::textds_flags());

        // read options
        const len_t threshold = env().option("threshold").as_integer(); //factor threshold
        lzss::FactorBuffer factors;

        {
            // Factorize
            env().begin_stat_phase("Factorize");

            strategy_t strategy(env().env_for_option("comp"));
            strategy.factorize(text, threshold, factors);

            env().log_stat("threshold", threshold);
            env().log_stat("factors", factors.size());
            env().end_stat_phase();
        }

        env().begin_stat_phase("Sorting Factors");
        // sort factors
        factors.sort();
        env().end_stat_phase();
        env().begin_stat_phase("Encode Factors");

        // encode
        typename coder_t::Encoder coder(env().env_for_option("coder"), output, lzss::TextLiterals<text_t>(text, factors));

        lzss::encode_text(coder, text, factors); //TODO is this correct?
        env().end_stat_phase();
    }

    inline virtual void decompress(Input& input, Output& output) override {
        //TODO: tell that forward-factors are allowed
        typename coder_t::Decoder decoder(env().env_for_option("coder"), input);
        auto outs = output.as_stream();


        //lzss::decode_text_internal<coder_t, dec_t>(decoder, outs);
        // if(lazy == 0)
        // 	lzss::decode_text_internal<coder_t, dec_t>(decoder, outs);
        // else
        lcpcomp::decode_text_internal<typename coder_t::Decoder, dec_t>(env().env_for_option("dec"), decoder, outs);
    }
};

}

