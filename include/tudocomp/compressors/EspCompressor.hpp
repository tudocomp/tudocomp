#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/compressors/esp/pre_header.hpp>
#include <tudocomp/compressors/esp/meta_blocks.hpp>
#include <tudocomp/compressors/esp/tree_reducer.hpp>

namespace tdc {

class EspCompressor: public Compressor {
    static const bool SILENT = true;

public:
    inline static Meta meta() {
        Meta m("compressor", "esp", "ESP based grammar compression");
        //m.option("coder").templated<coder_t>();
        //m.option("min_run").dynamic("3");
        return m;
    }

    using Compressor::Compressor;

    inline virtual void compress(Input& input, Output& output) override {
        using namespace esp;

        auto p1 = env().stat_phase("ESP Compressor");

        std::vector<std::vector<size_t>> g;

        {
            auto p2 = env().stat_phase("Input");
            auto in = input.as_view();

            auto p3 = env().stat_phase("ESP Algorithm");
                auto r = generate_grammar_rounds(in, SILENT);
            p3.end();

            auto p4 = env().stat_phase("Transform Grammar");
                //g = generate_grammar(r);
            p4.end();
        }

        auto out = output.as_stream();


    }

    inline virtual void decompress(Input& input, Output& output) override {

    }
};

}
