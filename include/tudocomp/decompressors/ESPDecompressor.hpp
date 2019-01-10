#pragma once

#include <tudocomp/compressors/esp/PlainSLPCoder.hpp>
#include <tudocomp/Decompressor.hpp>

#include <tudocomp_stat/StatPhase.hpp>

namespace tdc {

template<typename slp_coder_t>
class ESPDecompressor : public Decompressor {
public:
    inline static Meta meta() {
        Meta m(Decompressor::type_desc(), "esp", "ESP decompressor");
        m.param("slp_coder").strategy<slp_coder_t>(
            TypeDesc("slp_coder"), Meta::Default<esp::PlainSLPCoder>());
        return m;
    }

    using Decompressor::Decompressor;

    inline virtual void decompress(Input& input, Output& output) override {
        StatPhase phase0("ESP Decompressor");

        StatPhase phase1("Creating strategy");
        const slp_coder_t strategy { this->config().sub_config("slp_coder") };

        phase1.split("Decode SLP");
        auto slp = strategy.decode(input);

        phase1.split("Create output stream");
        auto out = output.as_stream();

        phase1.split("Derive text");
        if (!slp.is_empty()) {
            slp.derive_text(out);
        } else {
            out << ""_v;
        }
    }
};

}

