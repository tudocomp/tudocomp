#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/esp/SLP.hpp>
#include <tudocomp/compressors/esp/HashArray.hpp>

namespace tdc {namespace esp {
    class PlainSLPCoder: public Algorithm {
    public:
        inline static Meta meta() {
            Meta m("slp_coder", "plain");
            return m;
        };

        using Algorithm::Algorithm;

        inline void encode(SLP&& slp, Output& output) const {
            auto max_val = slp.size() - 1;
            auto bit_width = bits_for(max_val);

            BitOStream bout(output.as_stream());
            // Write header
            // bit width
            DCHECK_GE(bit_width, 1);
            DCHECK_LE(bit_width, 63); // 64-bit sizes are
                                    // restricted to 63 or less in practice

            if (slp.is_empty()) {
                bit_width = 0;
                DCHECK_EQ(slp.root_rule(), 0);
            }

            bout.write_int(bit_width, 6);

            // max_val and root rule
            bout.write_int(max_val, bit_width);
            bout.write_int(slp.root_rule(), bit_width);

            // Write rules
            for (size_t i = SLP_CODING_ALPHABET_SIZE; i < slp.size(); i++) {
                auto a = slp.get_l(i);
                auto b = slp.get_r(i);

                DCHECK_LE(a, max_val);
                DCHECK_LE(b, max_val);
                bout.write_int(a, bit_width);
                bout.write_int(b, bit_width);
            }
        }

        inline SLP decode(Input& input) const {
            BitIStream bin(input.as_stream());

            auto bit_width = bin.read_int<size_t>(6);
            bool empty = (bit_width == 0);

            auto max_val = bin.read_int<size_t>(bit_width);
            auto root_rule = bin.read_int<size_t>(bit_width);

            size_t slp_size = max_val + 1;

            //std::cout << "in:  Root rule: " << root_rule << "\n";

            esp::SLP slp { SLP_CODING_ALPHABET_SIZE };
            slp.set_empty(empty);
            slp.set_root_rule(root_rule);
            slp.reserve(slp_size);
            slp.resize(slp_size);

            size_t i = SLP_CODING_ALPHABET_SIZE;
            while (!bin.eof()) {
                auto a = bin.read_int<size_t>(bit_width);
                auto b = bin.read_int<size_t>(bit_width);

                slp.set(i++, a, b);
            }

            return slp;
        }
    };
}}
