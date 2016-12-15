#pragma once

#include <tudocomp/util.hpp>

#include <tudocomp/compressors/lzss/LZSSCoding.hpp>
#include <tudocomp/compressors/lzss/LZSSFactors.hpp>
#include <tudocomp/compressors/lzss/LZSSLiterals.hpp>

#include <tudocomp/compressors/esacomp/strategies/BulldozerStrategy.hpp>
#include <tudocomp/compressors/esacomp/strategies/LazyListStrategy.hpp>
#include <tudocomp/compressors/esacomp/strategies/MaxHeapStrategy.hpp>
#include <tudocomp/compressors/esacomp/strategies/MaxLCPStrategy.hpp>
#include <tudocomp/compressors/esacomp/strategies/NaiveStrategy.hpp>

#include <tudocomp/compressors/esacomp/decoding/DecodeQueueListBuffer.hpp>
#include <tudocomp/compressors/esacomp/decoding/SuccinctListBuffer.hpp>
#include <tudocomp/compressors/esacomp/decoding/MarvinBuffer.hpp>
#include <tudocomp/compressors/esacomp/decoding/MultiMapBuffer.hpp>

#include <tudocomp/ds/TextDS.hpp>

namespace tdc {
namespace esacomp {

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
   decoder.env().begin_stat_phase("Lazy EsaComp-Decoding");
    buffer.decode_lazy();
   decoder.env().end_stat_phase();
   decoder.env().begin_stat_phase("Eagerly EsaComp-Decoding");
    buffer.decode_eagerly();
    

    // log stats
    decoder.env().log_stat("longest_chain", buffer.longest_chain());

    // write decoded text
    buffer.write_to(outs);
   decoder.env().end_stat_phase();
}

}//ns

/// Factorizes the input by finding redundant phrases in a re-ordered version
/// of the LCP table.
template<typename coder_t, typename strategy_t, typename dec_t>
class ESACompressor : public Compressor {
private:
    typedef TextDS<> text_t;

public:
    inline static Meta meta() {
        Meta m("compressor", "esacomp");
        m.option("coder").templated<coder_t>();
        m.option("strategy").templated<strategy_t, esacomp::MaxLCPStrategy>();
        m.option("threshold").dynamic("3");
        m.option("esadec").templated<dec_t, esacomp::SuccinctListBuffer>();
        m.needs_sentinel_terminator();
        return m;
    }

    /// Construct the class with an environment.
    inline ESACompressor(Env&& env) : Compressor(std::move(env)) {}

    inline virtual void compress(Input& input, Output& output) override {
        auto in = input.as_view();
        DCHECK(in.ends_with(uint8_t(0)));
        text_t text(in, env());

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

        lzss::encode_text(coder, text, factors); //TODO is this correct?
    }

    inline virtual void decompress(Input& input, Output& output) override {
        //TODO: tell that forward-factors are allowed
        typename coder_t::Decoder decoder(env().env_for_option("coder"), input);
        auto outs = output.as_stream();


        //lzss::decode_text_internal<coder_t, dec_t>(decoder, outs);
        // if(lazy == 0)
        // 	lzss::decode_text_internal<coder_t, dec_t>(decoder, outs);
        // else
        esacomp::decode_text_internal<typename coder_t::Decoder, dec_t>(env().env_for_option("esadec"), decoder, outs);
    }
};

}

