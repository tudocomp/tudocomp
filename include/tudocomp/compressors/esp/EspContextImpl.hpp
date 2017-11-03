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

        std::unique_ptr<Round<ipd_t>> round_ptr;

        // Initialize initial round
        {
            auto phase = StatPhase("Prepare round 0");

            // TODO: Calc actual alphabet size, or make parametric over arbitrary alphabet
            size_t initial_alphabet_size = 256;

            round_ptr = std::make_unique<Round<ipd_t>>(Round<ipd_t> {
                GrammarRules<ipd_t>(initial_alphabet_size),
                initial_alphabet_size,
                IntVector<dynamic_t>(),
            });
            round_ptr->string.width(bits_for(initial_alphabet_size - 1));
            round_ptr->string.reserve(input.size(), bits_for(initial_alphabet_size - 1));
            for (auto c : input) {
                // TODO: Take input as a stream instead
                round_ptr->string.push_back(c);
            }
        }

        for(size_t n = 0;; n++) {
            std::stringstream ss;
            ss << "Round " << n;
            auto phase = StatPhase(ss.str());

            auto& round = *round_ptr;
            in_t in = round.string;

            esp::RoundContext<in_t> ctx {
                round.alphabet
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

            {
                auto block_grid = ctx.split_into_blocks(in);

                in_t s = in;
                block_grid.for_each_block_len([&](size_t block_len) {
                    auto slice = s.slice(0, block_len);
                    s = s.slice(block_len);
                    auto rule_name = round.gr.add(slice) - (round.gr.initial_counter() - 1);

                    auto old_cap = new_layer.capacity();
                    new_layer.push_back(rule_name);
                    auto new_cap = new_layer.capacity();
                    DCHECK_EQ(old_cap, new_cap);
                });
            }

            // Delete previous string
            round.string = IntVector<dynamic_t>();

            DCHECK_EQ(round.string.size(), 0);
            DCHECK_EQ(round.string.capacity(), 0);

            new_layer.shrink_to_fit();

            // Append to slp array
            {
                size_t old_slp_size = slp.rules.size();
                size_t additional_slp_size = round.gr.rules_count();
                size_t new_slp_size = old_slp_size + additional_slp_size;

                slp.rules.reserve(new_slp_size);
                slp.rules.resize(new_slp_size);

                auto& rv = slp.rules;

                round.gr.for_all([&](const auto& k, const auto& val_) {
                    const auto& val = val_ - round.gr.initial_counter();
                    const auto& key = k.as_view();

                    size_t store_idx = slp_counter + val - 256;
                    rv[store_idx][0] = key[0] + prev_slp_counter;
                    rv[store_idx][1] = key[1] + prev_slp_counter;
                });

                prev_slp_counter = slp_counter;
                slp_counter += additional_slp_size;
            }

            // carry over stats
            auto round_ipd_stats = round.gr.stats();
            ipd_stats.ext_size2_total += round_ipd_stats.ext_size2_total;
            ipd_stats.ext_size3_total += round_ipd_stats.ext_size3_total;
            ipd_stats.ext_size3_unique += round_ipd_stats.ext_size3_unique;
            ipd_stats.int_size2_total += round_ipd_stats.int_size2_total;
            ipd_stats.int_size2_unique += round_ipd_stats.int_size2_unique;

            // Delete previous hashmap
            round.gr.clear();

            // Prepare next round
            auto tmp = Round<ipd_t> {
                GrammarRules<ipd_t>(round.gr.rules_count()),
                round.gr.rules_count(),
                std::move(new_layer),
            };

            round_ptr.reset(); // Reset unique pointer to drop contained Round as soon as possible
            round_ptr = std::make_unique<Round<ipd_t>>(std::move(tmp));

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
