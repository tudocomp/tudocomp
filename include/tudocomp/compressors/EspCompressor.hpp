#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/compressors/esp/EspContextImpl.hpp>

namespace tdc {

class EspCompressor: public Compressor {
    static const bool SILENT = false;

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

        EspContext context;
        SLP slp;

        {
            auto p2 = env().stat_phase("Input");
            auto in = input.as_view();

            auto p3 = env().stat_phase("ESP Algorithm");
                auto r = context.generate_grammar_rounds(in, SILENT);
            p3.end();

            auto p4 = env().stat_phase("Generate SLP from Hashmaps");
                slp = context.generate_grammar(r);
            p4.end();
        }

        auto max_val = slp.rules.size() + esp::GRAMMAR_PD_ELLIDED_PREFIX - 1;
        std::cout << "OUT: max value: " << max_val << "\n";

        auto bit_width = bits_for(max_val);
        std::cout << "OUT: bits: " << int(bit_width) << "\n";

        std::cout << "OUT: Root rule: " << slp.root_rule << "\n";

        BitOStream bout(output.as_stream());
        // Write header
        // bit width
        DCHECK_GE(bit_width, 1);
        DCHECK_LE(bit_width, 63); // 64-bit sizes are
                                  // restricted to 63 or less in practice

        if (slp.empty) {
            bit_width = 0;
            DCHECK(slp.rules.empty());
            DCHECK_EQ(slp.root_rule, 0);
        }

        bout.write_int(bit_width, 6);

        // root rule
        bout.write_int(slp.root_rule, bit_width);

        // Write rules
        for (auto& rule : slp.rules) {
            //std::cout << "OUT: " << vec_to_debug_string(rule) << "\n";
            DCHECK_LE(rule[0], max_val);
            DCHECK_LE(rule[1], max_val);
            bout.write_int(rule[0], bit_width);
            bout.write_int(rule[1], bit_width);
        }
    }

    inline virtual void decompress(Input& input, Output& output) override {
        BitIStream bin(input.as_stream());

        auto bit_width = bin.read_int<size_t>(6);
        bool empty = (bit_width == 0);

        auto root_rule = bin.read_int<size_t>(bit_width);

        std::cout << "in:  Root rule: " << root_rule << "\n";

        esp::SLP slp;
        slp.root_rule = root_rule;
        slp.rules.reserve(std::pow(2, bit_width)); // TODO: Make more exact

        while (!bin.eof()) {
            auto a = bin.read_int<size_t>(bit_width);
            auto b = bin.read_int<size_t>(bit_width);
            auto array = std::array<size_t, 2>{{ a, b, }};

            //std::cout << "IN:  " << vec_to_debug_string(array) << "\n";

            slp.rules.push_back(array);
        }

        auto out = output.as_stream();
        if (!empty) {
            slp.derive_text(out);
        } else {
            out << ""_v;
        }
    }
};

}
