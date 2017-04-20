#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/esp/SLPDepSort.hpp>

namespace tdc {namespace esp {
    class SufficientSLPStrategy: public Algorithm {
    public:
        inline static Meta meta() {
            Meta m("slp_strategy", "sufficient");
            return m;
        };

        using Algorithm::Algorithm;

        inline void encode(EspContext& context, SLP&& slp, Output& output) const {
            slp_dep_sort(slp); // can be implemented better, and in a way that yields
                               // temporary lists for reusal

            auto max_val = slp.rules.size() + esp::GRAMMAR_PD_ELLIDED_PREFIX - 1;
            auto bit_width = bits_for(max_val);

            BitOStream bout(output.as_stream());

            DCHECK_GE(bit_width, 1);
            DCHECK_LE(bit_width, 63); // 64-bit sizes are
                                    // restricted to 63 or less in practice

            if (slp.empty) {
                bit_width = 0;
                DCHECK(slp.rules.empty());
                DCHECK_EQ(slp.root_rule, 0);
            }

            // bit width
            bout.write_int(bit_width, 6);

            // size
            bout.write_int(max_val, bit_width);

            // root rule
            bout.write_int(slp.root_rule, bit_width);

            // Write rules
            // ...
            size_t last = 0;
            for (auto& e : slp.rules) {
                DCHECK_LE(last, e[0]);
                size_t diff = e[0] - last;
                bout.write_unary(diff);
            }
        }

        inline SLP decode(Input& input) const {
            BitIStream bin(input.as_stream());

            // bit width
            auto bit_width = bin.read_int<size_t>(6);
            bool empty = (bit_width == 0);

            // size
            auto max_val = bin.read_int<size_t>(bit_width);

            // root rule
            auto root_rule = bin.read_int<size_t>(bit_width);

            //std::cout << "in:  Root rule: " << root_rule << "\n";

            esp::SLP slp;
            slp.empty = empty;
            slp.root_rule = root_rule;
            slp.rules.reserve(max_val + 1); // TODO: Make more exact

            std::vector<size_t> rules_lhs;
            size_t last = 0;
            for(size_t i = 0; i <= max_val && !bin.eof(); i++) {
                // ...
                auto value = bin.read_unary<size_t>() + last;
                last = value;
                rules_lhs.push_back(value);
            }

            return esp::SLP();
        }
    };
}}
