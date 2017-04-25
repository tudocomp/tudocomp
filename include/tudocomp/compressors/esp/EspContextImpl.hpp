#pragma once

#include <tudocomp/compressors/esp/EspContext.hpp>
#include <tudocomp/compressors/esp/RoundContext.hpp>
#include <tudocomp/compressors/esp/meta_blocks.hpp>

#include <tudocomp/compressors/esp/utils.hpp>

namespace tdc {namespace esp {
    Rounds EspContext::generate_grammar_rounds(string_ref input) {
        std::vector<Round> rounds;
        size_t root_node = 0;
        bool empty = false;


        // Initialize initial round
        {
            auto phase = with_env([&](auto& env) {
                return env.stat_phase("Prepare round 0");
            });

            Round round0 {
                GrammarRules(256),
                256, // TODO: Calc actual alphabet size
                std::vector<size_t>(),
            };
            round0.string.reserve(input.size());
            for (auto c : input) {
                round0.string.push_back(c);
            }
            rounds.push_back(std::move(round0));
        }

        for(size_t n = 0;; n++) {
            auto phase = with_env([&](auto& env) {
                std::stringstream ss;
                ss << "Round " << n;
                return env.stat_phase(ss.str());
            });

            Round& r = rounds.back();
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
                root_node = in[0];
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
                    auto rule_name = r.gr.add(slice) - (r.gr.m_initial_counter - 1);

                    ctx.debug.slice_symbol_map(slice, rule_name);

                    new_layer.push_back(rule_name);
                }
            }

            // Delete previous string
            r.string = std::vector<size_t>();

            // TODO: Preallocate based on number of rules
            new_layer.shrink_to_fit();

            // Prepare next round
            {
                rounds.push_back(Round {
                    GrammarRules(r.gr.rules_count()),
                    r.gr.rules_count(),
                    std::move(new_layer),
                });
            }
        }

        return Rounds {
            std::move(rounds),
            root_node,
            empty,
        };
    }

    ///
    /// REFACTOR: ONE GLOBAL HASHMAP STRUCTURE WITH GLOBAL ARRAY INDICE CONTENTS
    /// SUBTRACT DOWN TO LAYER_INDEX MANUALLY WHERE NEEDED
    ///
    /// NEW GOAL
    ///
    /// direct hashmap -> slp conversion, with push-to-end and
    /// remembering original start symbol

    // TODO: Either ellided-256 alphabet, or compressed included alphabet

    SLP EspContext::generate_grammar(const Rounds& rds) {
        auto& rs = rds.rounds;
        bool empty = rds.empty;

        size_t size2 = 0;
        size_t size3 = 0;
        for (auto& r: rs) {
            size2 += r.gr.n2.size();
            size3 += r.gr.n3.size();
        }

        std::vector<std::array<size_t, 2>> ret;

        {
            size_t vector_size = size2 + 2 * size3 + (256 - GRAMMAR_PD_ELLIDED_PREFIX);
            ret.reserve(vector_size);
            ret.resize(vector_size, std::array<size_t, 2>{{ 0,0 }});
        }

        size_t i3 = size2 + size3;

        size_t counter_offset = GRAMMAR_PD_ELLIDED_PREFIX;
        size_t prev_counter_offset = 0;

        size_t last_idx = rds.root_node;

        size_t debug_round = 0;
        for (auto& r: rs) {
            const size_t i3_end = i3 + r.gr.n3.size();

            auto doit = [&](auto& n) {
                // NB! Non determinism here!
                for (auto& kv: n) {
                    const auto& key = kv.first;
                    const auto& val = kv.second - r.gr.m_initial_counter;

                    size_t rule_idx = counter_offset + val;
                    size_t store_idx = rule_idx - GRAMMAR_PD_ELLIDED_PREFIX;

                    last_idx = std::max(rule_idx, last_idx);

                    DCHECK_EQ(ret.at(store_idx).at(0), 0);
                    DCHECK_EQ(ret.at(store_idx).at(1), 0);

                    if (key.m_data.size() == 2) {
                        ret[store_idx][0] = key.m_data[0] + prev_counter_offset;
                        ret[store_idx][1] = key.m_data[1] + prev_counter_offset;
                    } else if (key.m_data.size() == 3) {
                        ret[store_idx][0] = i3 + GRAMMAR_PD_ELLIDED_PREFIX;
                        ret[store_idx][1] = key.m_data[2] + prev_counter_offset;

                        ret[i3][0] = key.m_data[0] + prev_counter_offset;
                        ret[i3][1] = key.m_data[1] + prev_counter_offset;

                        i3++;
                    } else {
                        DCHECK(false);
                    }

                    DCHECK_NE(ret.at(store_idx).at(0), 0);
                    DCHECK_NE(ret.at(store_idx).at(1), 0);
                }
            };

            doit(r.gr.n2);
            doit(r.gr.n3);
            DCHECK_EQ(i3, i3_end);
            prev_counter_offset = counter_offset;
            counter_offset += r.gr.rules_count();

            debug_round++;
        }

        DCHECK_EQ(counter_offset, size2 + size3 + GRAMMAR_PD_ELLIDED_PREFIX);
        DCHECK_EQ(i3, ret.size());

        debug.generate_grammar(empty, last_idx);
        return SLP {
            std::move(ret),
            last_idx,
            empty,
        };
    }
}}
