#pragma once

#include <tudocomp/Algorithm.hpp>

namespace tdc {namespace esp {
    class PlainSLPStrategy: public Algorithm {
    public:
        inline static Meta meta() {
            Meta m("slp_strategy", "plain");
            return m;
        };

        using Algorithm::Algorithm;

        inline void encode(DebugContext& debug, SLP&& slp, Output& output) const {
            debug.encode_start();
            auto max_val = slp.rules.size() + esp::GRAMMAR_PD_ELLIDED_PREFIX - 1;
            auto bit_width = bits_for(max_val);
            debug.encode_max_value(max_val, bit_width);
            debug.encode_root_node(slp.root_rule);

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
            debug.encode_rule_start();
            for (auto& rule : slp.rules) {
                debug.encode_rule(rule);
                DCHECK_LE(rule[0], max_val);
                DCHECK_LE(rule[1], max_val);
                bout.write_int(rule[0], bit_width);
                bout.write_int(rule[1], bit_width);
            }
        }

        inline SLP decode(Input& input) const {
            BitIStream bin(input.as_stream());

            auto bit_width = bin.read_int<size_t>(6);
            bool empty = (bit_width == 0);

            auto root_rule = bin.read_int<size_t>(bit_width);

            //std::cout << "in:  Root rule: " << root_rule << "\n";

            esp::SLP slp;
            slp.empty = empty;
            slp.root_rule = root_rule;
            slp.rules.reserve(std::pow(2, bit_width)); // TODO: Make more exact

            while (!bin.eof()) {
                auto a = bin.read_int<size_t>(bit_width);
                auto b = bin.read_int<size_t>(bit_width);
                auto array = std::array<size_t, 2>{{ a, b, }};

                //std::cout << "IN:  " << vec_to_debug_string(array) << "\n";

                slp.rules.push_back(array);
            }

            return slp;
        }
    };
}}
