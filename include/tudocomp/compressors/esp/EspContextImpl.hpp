#pragma once

#include <tudocomp_stat/StatPhase.hpp>

#include <tudocomp/compressors/esp/EspContext.hpp>
#include <tudocomp/compressors/esp/RoundContext.hpp>
#include <tudocomp/compressors/esp/meta_blocks.hpp>

#include <tudocomp/compressors/esp/utils.hpp>

namespace tdc {namespace esp {
    template<typename ipd_t>
    SLP EspContext<ipd_t>::generate_grammar(string_ref input) {
        size_t root_node = 0;
        bool empty = false;

        SLP slp;
        size_t slp_counter = 256;
        size_t prev_slp_counter = 0;

        std::unique_ptr<Round<ipd_t>> round;

        // Initialize initial round
        {
            auto phase = with_env([&](auto& env) {
                return StatPhase("Prepare round 0");
            });

            round = std::make_unique<Round<ipd_t>>(Round<ipd_t> {
                GrammarRules<ipd_t>(256),
                256, // TODO: Calc actual alphabet size
                std::vector<size_t>(),
            });
            round->string.reserve(input.size());
            for (auto c : input) {
                round->string.push_back(c);
            }
        }

        for(size_t n = 0;; n++) {
            auto phase = with_env([&](auto& env) {
                std::stringstream ss;
                ss << "Round " << n;
                return StatPhase(ss.str());
            });

            auto& r = *round;
            in_t in = r.string;

            esp::RoundContext<in_t> ctx {
                r.alphabet,
                in,
                behavior_metablocks_maximimze_repeating,
                behavior_landmarks_tie_to_right,
                debug.round(),
            };

            ctx.debug.init(n, in, r.alphabet);

            if (in.size() == 0) {
                empty = true;
                ctx.debug.last_round(0, true);
                break;
            }
            if (in.size() == 1) {
                root_node = in[0] + prev_slp_counter;
                ctx.debug.last_round(root_node, false);
                break;
            }

            std::vector<size_t> new_layer;

            ctx.split(in);

            const auto& v = ctx.adjusted_blocks();

            ctx.debug.slice_symbol_map_start();
            {
                in_t s = in;
                for (auto e : v) {
                    auto slice = s.slice(0, e.len);
                    s = s.slice(e.len);
                    auto rule_name = r.gr.add(slice) - (r.gr.initial_counter() - 1);

                    ctx.debug.slice_symbol_map(slice, rule_name);

                    new_layer.push_back(rule_name);
                }
            }

            // Delete previous string
            r.string = std::vector<size_t>();

            DCHECK_EQ(r.string.size(), 0);
            DCHECK_EQ(r.string.capacity(), 0);

            // TODO: Preallocate based on number of rules
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

            phase.log_stat("slp size", slp.rules.size());
        }

        slp.empty = empty;
        slp.root_rule = root_node;

        return slp;
    }
}}
