#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/esp/SLPDepSort.hpp>
#include <tudocomp/compressors/esp/MonotoneSubsequences.hpp>
#include <tudocomp/compressors/esp/DRCoder.hpp>

namespace tdc {namespace esp {
    template<typename d_coding_t = DMonotonSubseq<>>
    class SortedSLPCoder: public Algorithm {
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
            for (size_t i = 0; i < slp.size(); i++) {
                std::cout
                    << i
                    << " -> (" << slp.get_l(i) << ", " << slp.get_r(i) << ")\n";
            }
            */

            auto phase = StatPhase("SLP sort");
            slp_dep_sort(slp); // can be implemented better, and in a way that yields
                               // temporary lists for reusal

            if (env().option("dump_json").as_bool()) {
                phase.split("Dump JSON");
                std::ofstream ostream(env().option("dump_json_file").as_string() + ".json");

                auto& dl = slp.dl();
                auto& dr = slp.dr();

                ostream << "{\n";
                ostream << "    \"DL\" : " << vec_to_debug_string(dl) << "\n,";
                ostream << ",\n";
                ostream << "    \"DR\" : " << vec_to_debug_string(dr) << "\n";
                ostream << "}\n";

                ostream.flush();
                ostream.close();
            }

            phase.split("Encode headers");

            auto max_val = slp.size() - 1;
            auto bit_width = bits_for(max_val);

            auto bouts = std::make_shared<BitOStream>(output.as_stream());
            auto& bout = *bouts;

            DCHECK_GE(bit_width, 1);
            DCHECK_LE(bit_width, 63); // 64-bit sizes are
                                    // restricted to 63 or less in practice

            if (slp.is_empty()) {
                bit_width = 0;
                DCHECK_EQ(slp.root_rule(), 0);
            }

            // bit width
            bout.write_int(bit_width, 6);

            // size
            bout.write_int(max_val, bit_width);

            // root rule
            bout.write_int(slp.root_rule(), bit_width);

            if (slp.is_empty() || slp.root_rule() < SLP_CODING_ALPHABET_SIZE) {
                return;
            }

            /*
            std::cout << "sorted:\n";
            for (size_t i = 0; i < slp.size(); i++) {
                std::cout
                    << i
                    << " -> (" << slp.get_l(i) << ", " << slp.get_r(i) << ")\n";
            }
            */

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
                auto dl = std::move(slp.dl());

                size_t last = 0;
                for (size_t node : dl) {
                    DCHECK_LE(last, node);
                    size_t diff = node - last;
                    bout.write_unary(diff);
                    last = node;
                }
            }

            phase.split("Encode SLP RHS");

            const auto dr = std::move(slp.dr());
            d_coding_t d_coding { this->env().env_for_option("d_coding") };
            d_coding.encode(dr, bouts, bit_width, max_val);
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

            esp::SLP slp { SLP_CODING_ALPHABET_SIZE };
            slp.set_empty(empty);
            slp.set_root_rule(root_rule);

            if (empty || slp.root_rule() < SLP_CODING_ALPHABET_SIZE) {
                return slp;
            }

            size_t slp_size = max_val + 1;
            slp.reserve(slp_size);
            slp.resize(slp_size);

            // Read rules lhs
            {
                size_t last = 0;
                for(size_t i = 0; i < slp_size && !bin.eof(); i++) {
                    // ...
                    auto diff = bin.read_unary<size_t>();
                    last += diff;
                    slp.set_l(i, last);
                }
            }

            d_coding_t d_coding { this->env().env_for_option("d_coding") };
            d_coding.decode(slp.dr(), bins, bit_width, max_val);

            return slp;
        }
    };
}}
