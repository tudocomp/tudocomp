#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/esp/SLPDepSort.hpp>
#include <tudocomp/compressors/esp/MonotoneSubsequences.hpp>
#include <tudocomp/compressors/esp/DRCoder.hpp>

namespace tdc {namespace esp {
    template<typename d_coding_t = DMonotonSubseq<>>
    class SortedSLPCoder: public Algorithm {
        using RhsAdapter = SLPRhsAdapter;
    public:
        inline static Meta meta() {
            Meta m("slp_coder", "sorted");
            m.option("d_coding").templated<d_coding_t, DMonotonSubseq<>>("d_coding");
            m.option("dump_json").dynamic("false");
            m.option("dump_json_file").dynamic("-");
            return m;
        };

        using Algorithm::Algorithm;

        inline void encode(SLP&& slp, Output& output) const {
            /*
            std::cout << "unsorted:\n";
            for (size_t i = 0; i < slp.rules.size(); i++) {
                std::cout
                    << i << ": "
                    << i + esp::GRAMMAR_PD_ELLIDED_PREFIX
                    << " -> (" << slp.rules[i][0] << ", " << slp.rules[i][1] << ")\n";
            }
            */

            auto phase = StatPhase("SLP sort");
            slp_dep_sort(slp); // can be implemented better, and in a way that yields
                               // temporary lists for reusal

            if (env().option("dump_json").as_bool()) {
                phase.split("Dump JSON");
                std::ofstream ostream(env().option("dump_json_file").as_string() + ".json");

                std::vector<size_t> dl;
                std::vector<size_t> dr;
                for (auto& e : slp.rules) {
                    dl.push_back(e[0]);
                    dr.push_back(e[1]);
                }
                ostream << "{\n";
                ostream << "    \"DL\" : " << vec_to_debug_string(dl) << "\n,";
                ostream << ",\n";
                ostream << "    \"DR\" : " << vec_to_debug_string(dr) << "\n";
                ostream << "}\n";

                ostream.flush();
                ostream.close();
            }

            phase.split("Encode headers");

            auto max_val = slp.rules.size() + esp::GRAMMAR_PD_ELLIDED_PREFIX - 1;
            auto bit_width = bits_for(max_val);

            auto bouts = std::make_shared<BitOStream>(output.as_stream());
            auto& bout = *bouts;

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

            if (slp.empty || slp.root_rule < 256) {
                return;
            }

            /*
            std::cout << "sorted:\n";
            for (size_t i = 0; i < slp.rules.size(); i++) {
                std::cout
                    << i << ": "
                    << i + esp::GRAMMAR_PD_ELLIDED_PREFIX
                    << " -> (" << slp.rules[i][0] << ", " << slp.rules[i][1] << ")\n";
            }*/

            // ...
            //std::vector<size_t> rules_lhs;
            //std::vector<size_t> rules_lhs_diff;
            //rules_lhs.push_back(e[0]);
            //rules_lhs_diff.push_back(diff);
            //std::cout << "emitted lhs:   " << vec_to_debug_string(rules_lhs) << "\n";
            //std::cout << "emitted diffs: " << vec_to_debug_string(rules_lhs_diff) << "\n";

            phase.split("Encode SLP LHS");

            // Write rules lhs
            {
                size_t last = 0;
                for (auto& e : slp.rules) {
                    DCHECK_LE(last, e[0]);
                    size_t diff = e[0] - last;
                    bout.write_unary(diff);
                    last = e[0];
                }
            }

            phase.split("Encode SLP RHS");

            const auto rhs = RhsAdapter { &slp };
            d_coding_t d_coding { this->env().env_for_option("d_coding") };
            d_coding.encode(rhs, bouts, bit_width, max_val);
        }

        inline SLP decode(Input& input) const {
            auto bins = std::make_shared<BitIStream>(input.as_stream());
            auto& bin = *bins;

            // bit width
            auto bit_width = bin.read_int<size_t>(6);
            bool empty = (bit_width == 0);

            // size
            auto max_val = bin.read_int<size_t>(bit_width);

            // root rule
            auto root_rule = bin.read_int<size_t>(bit_width);

            //std::cout << "in:  Root rule: " << root_rule << "\n";
            //std::vector<size_t> rules_lhs_diff;
            //rules_lhs_diff.push_back(diff);
            //std::cout << "parsed lhs:   " << vec_to_debug_string(rules_lhs) << "\n";
            //std::cout << "parsed diffs: " << vec_to_debug_string(rules_lhs_diff) << "\n";

            esp::SLP slp;
            slp.empty = empty;
            slp.root_rule = root_rule;

            if (empty || slp.root_rule < 256) {
                return slp;
            }

            size_t slp_size = (max_val + 1) - 256; // implied initial bytes
            slp.rules.reserve(slp_size);
            slp.rules.resize(slp_size);

            // Read rules lhs
            {
                size_t last = 0;
                for(size_t i = 0; i < slp_size && !bin.eof(); i++) {
                    // ...
                    auto diff = bin.read_unary<size_t>();
                    last += diff;
                    slp.rules[i][0] = last;
                }
            }

            auto D = RhsAdapter { &slp };
            d_coding_t d_coding { this->env().env_for_option("d_coding") };
            d_coding.decode(D, bins, bit_width, max_val);

            return slp;
        }
    };
}}
