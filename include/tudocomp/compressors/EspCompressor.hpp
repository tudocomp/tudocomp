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
            auto p2 = env().stat_phase("Input");
            auto in = input.as_view();

            context.debug.input_string(in);

            auto p3 = env().stat_phase("ESP Algorithm");
                auto r = context.generate_grammar_rounds(in);
            p3.end();

            auto p4 = env().stat_phase("Generate SLP from Hashmaps");
                slp = context.generate_grammar(r);
            p4.end();
        }

        const slp_strategy_t strategy { this->env().env_for_option("slp_strategy") };

        strategy.encode(context, std::move(slp), output);

        context.debug.print_all();
    }

    inline virtual void decompress(Input& input, Output& output) override {
        const slp_strategy_t strategy { this->env().env_for_option("slp_strategy") };

        auto slp = strategy.decode(input);

        auto out = output.as_stream();
        if (!slp.empty) {
            slp.derive_text(out);
        } else {
            out << ""_v;
        }
    }
};

}
