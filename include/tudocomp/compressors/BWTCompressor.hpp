#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/ds/bwt.hpp>
#include <tudocomp/ds/TextDS.hpp>
#include <tudocomp/util.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {

template<typename text_t = TextDS<>>
class BWTCompressor : public Compressor {
public:
    inline static Meta meta() {
        Meta m(Compressor::type_desc(), "bwt",
            "Computes the Burrows-Wheeler transform of the input text.");
        m.param("textds", "The text data structure provider.")
            .strategy<text_t>(TypeDesc("textds"), Meta::Default<TextDS<>>());
        m.uses_textds<text_t>(ds::SA);
        return m;
    }

    using Compressor::Compressor;

    inline virtual void compress(Input& input, Output& output) override {
        auto ostream = output.as_stream();
        auto in = input.as_view();
        DCHECK(in.ends_with(uint8_t(0)));

        text_t t(config().sub_config("textds"), in, text_t::SA);
		DVLOG(2) << vec_to_debug_string(t);
		const len_t input_size = t.size();

        StatPhase::wrap("Construct Text DS", [&]{
            t.require(text_t::SA);
            DVLOG(2) << vec_to_debug_string(t.require_sa());
        });

        const auto& sa = t.require_sa();
        for(size_t i = 0; i < input_size; ++i) {
            ostream << bwt::bwt(t,sa,i);
        }
    }

    inline virtual void decompress(Input& input, Output& output) override {
        auto in = input.as_view();
        auto ostream = output.as_stream();

		auto decoded_string = StatPhase::wrap("Decode BWT", [&]{
            return bwt::decode_bwt(in);
        });

		if(tdc_unlikely(decoded_string.empty())) {
			return;
		}

        StatPhase::wrap("Output Text", [&]{
            ostream << decoded_string << '\0';
        });
    }
};

}//ns

