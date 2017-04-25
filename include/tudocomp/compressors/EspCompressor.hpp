#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/compressors/esp/EspContextImpl.hpp>
#include <tudocomp/compressors/esp/RoundContextImpl.hpp>

#include <tudocomp/compressors/esp/PlainSLPStrategy.hpp>

namespace tdc {

template<typename slp_strategy_t>
class EspCompressor: public Compressor {
public:
    inline static Meta meta() {
        Meta m("compressor", "esp", "ESP based grammar compression");
        m.option("slp_strategy").templated<slp_strategy_t, esp::PlainSLPStrategy>();
        return m;
    }

    using Compressor::Compressor;

    inline virtual void compress(Input& input, Output& output) override {
        using namespace esp;

        auto p1 = env().stat_phase("ESP Compressor");

        EspContext context { &env(), true };
        SLP slp;

        {
            auto x = env().stat_phase("Compress Phase");

            auto p2 = env().stat_phase("Creating input view");
                auto in = input.as_view();
            p2.end();

            context.debug.input_string(in);

            auto p3 = env().stat_phase("ESP Algorithm");
                auto r = context.generate_grammar_rounds(in);
            p3.end();

            auto p4 = env().stat_phase("Generate SLP from Hashmaps");
                slp = context.generate_grammar(r);
            p4.end();
        }

        {
            auto x = env().stat_phase("Encode Phase");

            auto p5 = env().stat_phase("Creating strategy");
                const slp_strategy_t strategy { this->env().env_for_option("slp_strategy") };
            p5.end();

            auto p6 = env().stat_phase("Encode SLP");
                strategy.encode(context, std::move(slp), output);
            p6.end();
        }

        context.debug.print_all();
    }

    inline virtual void decompress(Input& input, Output& output) override {
        auto p1 = env().stat_phase("ESP Decompressor");

        auto p5 = env().stat_phase("Creating strategy");
            const slp_strategy_t strategy { this->env().env_for_option("slp_strategy") };
        p5.end();

        auto x = env().stat_phase("Decode SLP");
            auto slp = strategy.decode(input);
        x.end();

        auto y = env().stat_phase("Create output stream");
            auto out = output.as_stream();
        y.end();

        auto z = env().stat_phase("Derive text");
            if (!slp.empty) {
                slp.derive_text(out);
            } else {
                out << ""_v;
            }
        z.end();
    }
};

}
