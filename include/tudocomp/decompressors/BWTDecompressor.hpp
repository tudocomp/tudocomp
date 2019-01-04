#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/ds/bwt.hpp>
#include <tudocomp/Decompressor.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {

class BWTDecompressor : public Decompressor {
public:
    inline static Meta meta() {
        Meta m(Decompressor::type_desc(), "bwt",
            "Reverts the Burrows-Wheeler of a text.");
        return m;
    }

    using Decompressor::Decompressor;

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
            // FIXME: why is this omited by the BWT encoder?
            ostream << decoded_string << '\0';
        });
    }
};

}

