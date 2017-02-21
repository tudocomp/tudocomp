#pragma once

#include <tudocomp/compressors/esp/EspContext.hpp>
#include <tudocomp/compressors/esp/Context.hpp>
#include <tudocomp/compressors/esp/meta_blocks.hpp>

#include <tudocomp/compressors/esp/utils.hpp>

namespace tdc {namespace esp {
    Rounds EspContext::generate_grammar_rounds(string_ref input,
                                               bool silent = false) {
        std::vector<Round> rounds;
        size_t root_rule = 0;
        bool empty = false;

        // Initialize initial round
        {
            Round round0 {
                GrammarRules(),
                256,
                std::vector<size_t>(),
            };
            round0.string.reserve(input.size());
            for (auto c : input) {
                round0.string.push_back(c);
            }
            rounds.push_back(std::move(round0));
        }

        for(size_t n = 0;; n++) {
            if (!silent) std::cout << "\n[Round " << n << "]:\n\n";
            Round& r = rounds.back();

            if (r.string.size() == 0) {
                empty = true;
                break;
            }
            if (r.string.size() == 1) {
                if (!silent) std::cout << "Done\n";
                root_rule = r.string[0];
                break;
            }

            in_t in = r.string;
            std::vector<size_t> new_layer;

            esp::Context<in_t> ctx {
                r.alphabet,
                in,
            };
            if (!silent) {
                ctx.print_mb2_trace = false;
                ctx.print_only_adjusted = true;
            } else {
                ctx.print_mb2_trace = false;
                ctx.print_only_adjusted = true;
                ctx.print_mb_trace = false;
            }

            esp::split(in, ctx);
            if (!silent) std::cout << "[Adjusted]:\n";
            const auto& v = ctx.adjusted_blocks();

            if (!silent) std::cout << "\n[Rules]:\n";

            {
                in_t s = in;
                for (auto e : v) {
                    auto slice = s.slice(0, e.len);
                    s = s.slice(e.len);
                    if (!silent) std::cout <<  "  "
                        << debug_p(slice, r.alphabet);

                    auto rule_name = r.gr.add(slice);
                    new_layer.push_back(rule_name);
                    if (!silent) std::cout << " -> " << rule_name << "\n";
                }
            }
            new_layer.shrink_to_fit();

            // Prepare next round
            {
                rounds.push_back(Round {
                    GrammarRules(),
                    r.gr.counter,
                    std::move(new_layer),
                });
            }
        }

        std::cout << "empty: " << int(empty) << ", root_rule: " << int(root_rule) << "\n";
        return Rounds {
            std::move(rounds),
            root_rule,
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

        size_t vector_size =
            size2 + 2 * size3 + (256 - GRAMMAR_PD_ELLIDED_PREFIX);
        ret.reserve(vector_size);
        ret.resize(vector_size, std::array<size_t, 2>{{ 0,0 }});

        size_t i3 = size2 + size3;

        size_t counter_offset = GRAMMAR_PD_ELLIDED_PREFIX;
        size_t prev_counter_offset = 0;

        size_t last_idx = rds.root_rule;

        size_t debug_round = 0;
        for (auto& r: rs) {
            auto doit = [&](auto& n) {
                for (auto& kv: n) {
                    const auto& key = kv.first;
                    const auto& val = kv.second - 1;

                    size_t rule_idx = counter_offset + val;
                    size_t store_idx = rule_idx - GRAMMAR_PD_ELLIDED_PREFIX;

                    last_idx = rule_idx;

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
            prev_counter_offset = counter_offset;
            counter_offset += r.gr.counter - 1;

            debug_round++;
        }

        DCHECK_EQ(counter_offset, size2 + size3 + GRAMMAR_PD_ELLIDED_PREFIX);
        DCHECK_EQ(i3, ret.size());

        std::cout << "empty: " << int(empty) << ", root_rule: " << int(last_idx) << "\n";
        return SLP {
            std::move(ret),
            last_idx,
            empty,
        };
    }
}}
