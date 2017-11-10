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

private:
//    const TypeRange<len_t> len_r = TypeRange<len_t>();

public:
    inline static Meta meta() {
        Meta m("compressor", "bwt", "BWT Compressor");
        m.option("textds").templated<text_t, TextDS<>>("textds");
        m.uses_textds<text_t>(ds::SA);
        return m;
    }

    using Compressor::Compressor;

    inline virtual void compress(Input& input, Output& output) override {
        auto ostream = output.as_stream();
        auto in = input.as_view();
        DCHECK(in.ends_with(uint8_t(0)));

        text_t t(env().env_for_option("textds"), in, text_t::SA);
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

