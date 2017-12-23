#pragma once

#include <tudocomp/ds/BitPackingVectorSlice.hpp>

#include <tudocomp_stat/StatPhase.hpp>

#include <tudocomp/util.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/compressors/esp/EspContext.hpp>
#include <tudocomp/compressors/esp/PlainSLPCoder.hpp>
#include <tudocomp/compressors/esp/StdUnorderedMapIPD.hpp>

namespace tdc {

template<typename slp_coder_t, typename ipd_t = esp::StdUnorderedMapIPD>
class EspCompressor: public Compressor {
public:
    inline static Meta meta() {
        Meta m("compressor", "esp", "ESP based grammar compression");
        m.option("slp_coder").templated<slp_coder_t, esp::PlainSLPCoder>("slp_coder");
        m.option("ipd").templated<ipd_t, esp::StdUnorderedMapIPD>("ipd");
        return m;
    }

    using Compressor::Compressor;

    inline virtual void compress(Input& input, Output& output) override {
        using namespace esp;

        auto phase0 = StatPhase("ESP Compressor");

        EspContext<ipd_t> context;
        SLP slp { SLP_CODING_ALPHABET_SIZE };

        {
            auto phase1 = StatPhase("Compress Phase");

            auto phase2 = StatPhase("Creating input handler");
            auto in_stream = input.as_stream();
            size_t in_size = input.size();

            phase2.split("ESP Algorithm");

            // ## Actual ESP Algorithm runs here ## //
            slp = context.generate_grammar(in_stream.begin(), in_stream.end(), in_size, 256);
        }

        phase0.log_stat("SLP size", slp.size());
        phase0.log_stat("ext_size2_total", context.ipd_stats.ext_size2_total);
        phase0.log_stat("ext_size3_total", context.ipd_stats.ext_size3_total);
        phase0.log_stat("ext_size3_unique", context.ipd_stats.ext_size3_unique);
        phase0.log_stat("int_size2_total", context.ipd_stats.int_size2_total);
        phase0.log_stat("int_size2_unique", context.ipd_stats.int_size2_unique);

        {
            auto phase1 = StatPhase("Encode Phase");

            auto phase2 = StatPhase("Creating strategy");
            const slp_coder_t strategy { this->env().env_for_option("slp_coder") };

            phase2.split("Encode SLP");
            strategy.encode(std::move(slp), output);
        }
    }

    inline virtual void decompress(Input& input, Output& output) override {
        auto phase0 = StatPhase("ESP Decompressor");

        auto phase1 = StatPhase("Creating strategy");
        const slp_coder_t strategy { this->env().env_for_option("slp_coder") };

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
