#pragma once

#include <tudocomp_stat/StatPhase.hpp>

#include <tudocomp/compressors/esp/EspContext.hpp>
#include <tudocomp/compressors/esp/RoundContext.hpp>
#include <tudocomp/compressors/esp/meta_blocks.hpp>

#include <tudocomp/compressors/esp/utils.hpp>

namespace tdc {namespace esp {
    template<typename ipd_t>
    template<typename T>
    SLP EspContext<ipd_t>::generate_grammar(T&& input) {
        size_t root_node = 0;
        bool empty = false;

        SLP slp;
        size_t slp_counter = 256;
        size_t prev_slp_counter = 0;

        std::unique_ptr<Round<ipd_t>> round;

        // Initialize initial round
        {
            auto phase = StatPhase("Prepare round 0");

            round = std::make_unique<Round<ipd_t>>(Round<ipd_t> {
                GrammarRules<ipd_t>(256),
                256, // TODO: Calc actual alphabet size
                IntVector<dynamic_t>(),
            });
            round->string.width(bits_for(256 - 1));
            round->string.reserve(input.size(), bits_for(256 - 1));
            for (auto c : input) {
                round->string.push_back(c);
            }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
            auto discard = std::move(input);
#pragma GCC diagnostic pop
        }

        for(size_t n = 0;; n++) {
            std::stringstream ss;
            ss << "Round " << n;
            auto phase = StatPhase(ss.str());

            auto& r = *round;
            in_t in = r.string;

            esp::RoundContext<in_t> ctx {
                r.alphabet,
                in
            };

            if (in.size() == 0) {
                empty = true;
                break;
            }
            if (in.size() == 1) {
                root_node = in[0] + prev_slp_counter;
                break;
            }

            IntVector<dynamic_t> new_layer;
            size_t new_layer_width = bits_for(in.size() - 1);
            new_layer.width(new_layer_width);
            new_layer.reserve(in.size() / 2 + 1, new_layer_width);

            ctx.split(in);

            const auto& v = ctx.adjusted_blocks();

            {
                in_t s = in;
                for (auto e : v) {
                    auto slice = s.slice(0, e.len);
                    s = s.slice(e.len);
                    auto rule_name = r.gr.add(slice) - (r.gr.initial_counter() - 1);

                    auto old_cap = new_layer.capacity();
                    new_layer.push_back(rule_name);
                    auto new_cap = new_layer.capacity();
                    DCHECK_EQ(old_cap, new_cap);
                }
            }

            // Delete previous string
            r.string = IntVector<dynamic_t>();

            DCHECK_EQ(r.string.size(), 0);
            DCHECK_EQ(r.string.capacity(), 0);

            new_layer.shrink_to_fit();

            // Append to slp array
            {
                size_t old_slp_size = slp.rules.size();
                size_t additional_slp_size = r.gr.rules_count();
                size_t new_slp_size = old_slp_size + additional_slp_size;

                slp.rules.reserve(new_slp_size);
                slp.rules.resize(new_slp_size);

                auto& rv = slp.rules;

                r.gr.for_all([&](const auto& k, const auto& val_) {
                    const auto& val = val_ - r.gr.initial_counter();
                    const auto& key = k.as_view();

                    size_t store_idx = slp_counter + val - 256;
                    rv[store_idx][0] = key[0] + prev_slp_counter;
                    rv[store_idx][1] = key[1] + prev_slp_counter;
                });

                prev_slp_counter = slp_counter;
                slp_counter += additional_slp_size;
            }

            // carry over stats
            auto round_ipd_stats = r.gr.stats();
            ipd_stats.ext_size2_total += round_ipd_stats.ext_size2_total;
            ipd_stats.ext_size3_total += round_ipd_stats.ext_size3_total;
            ipd_stats.ext_size3_unique += round_ipd_stats.ext_size3_unique;
            ipd_stats.int_size2_total += round_ipd_stats.int_size2_total;
            ipd_stats.int_size2_unique += round_ipd_stats.int_size2_unique;

            // Delete previous hashmap
            r.gr.clear();

            // Prepare next round
            auto tmp = Round<ipd_t> {
                GrammarRules<ipd_t>(r.gr.rules_count()),
                r.gr.rules_count(),
                std::move(new_layer),
            };

            round.reset();
            round = std::make_unique<Round<ipd_t>>(std::move(tmp));

            phase.log_stat("SLP size", slp.rules.size());
            phase.log_stat("ext_size2_total", round_ipd_stats.ext_size2_total);
            phase.log_stat("ext_size3_total", round_ipd_stats.ext_size3_total);
            phase.log_stat("ext_size3_unique", round_ipd_stats.ext_size3_unique);
            phase.log_stat("int_size2_total", round_ipd_stats.int_size2_total);
            phase.log_stat("int_size2_unique", round_ipd_stats.int_size2_unique);
        }

        slp.empty = empty;
        slp.root_rule = root_node;

        return slp;
    }
}}
