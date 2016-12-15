#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/ds/bwt.hpp>
#include <tudocomp/ds/TextDS.hpp>
#include <tudocomp/util.hpp>

namespace tdc {

class BWTCompressor : public Compressor {

private:
//    const TypeRange<len_t> len_r = TypeRange<len_t>();

public:
    inline static Meta meta() {
        Meta m("compressor", "bwt", "BWT Compressor");
        m.needs_sentinel_terminator();
        return m;
    }

    inline BWTCompressor(Env&& env) : Compressor(std::move(env)) {
    }

    inline virtual void compress(Input& input, Output& output) override {
        auto ostream = output.as_stream();
        auto in = input.as_view();
        DCHECK(in.ends_with(uint8_t(0)));

        TextDS<> t(in, env());
		tdc_debug(VLOG(2) << vec_to_debug_string(t));
		const len_t input_size = t.size();

        env().begin_stat_phase("Construct Text DS");
        t.require(TextDS<>::SA);
		tdc_debug(VLOG(2) << vec_to_debug_string(t.require_sa()));
        env().end_stat_phase();
        const auto& sa = t.require_sa();
        for(size_t i = 0; i < input_size; ++i) {
            ostream << bwt::bwt(t,sa,i);
        }
    }

    inline virtual void decompress(Input& input, Output& output) override {
        auto in = input.as_view();
        auto ostream = output.as_stream();

        env().begin_stat_phase("Decode BWT");
		uliteral_t* decoded_string = bwt::decode_bwt(in);
        env().end_stat_phase();
		if(tdc_unlikely(decoded_string == nullptr)) {
			return;
		}
        env().begin_stat_phase("Output Text");
		ostream << decoded_string << '\0';
		delete [] decoded_string;
        env().end_stat_phase();
    }
};

}//ns

