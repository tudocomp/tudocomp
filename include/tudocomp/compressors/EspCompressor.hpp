#pragma once

#include <tudocomp/ds/BitPackingVectorSlice.hpp>

#include <tudocomp_stat/StatPhase.hpp>

#include <tudocomp/util.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/compressors/esp/EspContext.hpp>
#include <tudocomp/compressors/esp/PlainSLPCoder.hpp>
#include <tudocomp/compressors/esp/StdUnorderedMapIPD.hpp>

#include <tudocomp/decompressors/ESPDecompressor.hpp>

namespace tdc {

template<typename slp_coder_t, typename ipd_t = esp::StdUnorderedMapIPD>
class EspCompressor: public Compressor {
public:
    inline static Meta meta() {
        Meta m(Compressor::type_desc(), "esp", "ESP based grammar compression");
        m.param("slp_coder").strategy<slp_coder_t>(TypeDesc("slp_coder"), Meta::Default<esp::PlainSLPCoder>());
        m.param("ipd").strategy<ipd_t>(TypeDesc("ipd"), Meta::Default<esp::StdUnorderedMapIPD>());
        return m;
    }

    using Compressor::Compressor;

    inline virtual void compress(Input& input, Output& output) override {
        using namespace esp;

        StatPhase phase0("ESP Compressor");

        EspContext<ipd_t> context;
        SLP slp { SLP_CODING_ALPHABET_SIZE };

        {
            StatPhase phase1("Compress Phase");

            StatPhase phase2("Creating input handler");
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
            StatPhase phase1 ("Encode Phase");

            StatPhase phase2("Creating strategy");
            const slp_coder_t strategy { this->config().sub_config("slp_coder") };

            phase2.split("Encode SLP");
            strategy.encode(std::move(slp), output);
        }
    }

    inline std::unique_ptr<Decompressor> decompressor() const override {
        return Algorithm::instance<ESPDecompressor<slp_coder_t>>();
    }
};

}
