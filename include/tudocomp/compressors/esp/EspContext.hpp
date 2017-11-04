#pragma once

#include <tudocomp_stat/StatPhase.hpp>

#include <tudocomp/util/View.hpp>

#include <tudocomp/compressors/esp/SLP.hpp>
#include <tudocomp/compressors/esp/GrammarRules.hpp>
#include <tudocomp/compressors/esp/LevelContext.hpp>
#include <tudocomp/compressors/esp/meta_blocks.hpp>
#include <tudocomp/compressors/esp/utils.hpp>

namespace tdc {namespace esp {
    using dynamic_bit_vector_t = DynamicIntVector;
    using dynamic_bit_view_t = BitPackingVectorSlice<dynamic_t>; // TODO: Overhaul tudocomp view types

    template<typename ipd_t>
    struct Level {
        GrammarRules<ipd_t> gr;
        size_t alphabet;
        dynamic_bit_vector_t string;
    };

    template<typename ipd_t>
    class EspContext {
    public:
        IPDStats ipd_stats;

        EspContext() = default;

        template<typename iterator_t>
        SLP generate_grammar(iterator_t&& begin, iterator_t&& end,
                             size_t size, size_t initial_alphabet_size) {
            size_t root_node = 0;
            bool empty = false;

            SLP slp;
            size_t slp_counter = initial_alphabet_size;
            size_t prev_slp_counter = 0;

            std::unique_ptr<Level<ipd_t>> level_ptr;

            // Copy the input into the level buffer for level 0,
            // and set the initial alphabet size
            {
                auto phase = StatPhase("Prepare level 0");

                level_ptr = std::make_unique<Level<ipd_t>>(Level<ipd_t> {
                    GrammarRules<ipd_t>(initial_alphabet_size),
                    initial_alphabet_size,
                    dynamic_bit_vector_t(),
                });
                size_t bit_width = bits_for(initial_alphabet_size - 1);
                level_ptr->string.width(bit_width);
                level_ptr->string.reserve(size, bit_width);
                for (; begin != end; ++begin) {
                    level_ptr->string.push_back(*begin);
                }
            }

            // Iteratively generate new levels until we end up with a level with just
            // 1 symbol
            for(size_t n = 0;; n++) {
                std::stringstream ss;
                ss << "Level " << n;
                auto phase = StatPhase(ss.str());

                auto& level = *level_ptr;
                dynamic_bit_view_t level_str = level.string;

                LevelContext ctx {
                    level.alphabet
                };

                if (level_str.size() == 0) {
                    empty = true;
                    break;
                }
                if (level_str.size() == 1) {
                    root_node = level_str[0] + prev_slp_counter;
                    break;
                }

                dynamic_bit_vector_t new_level_str;
                size_t new_level_str_width = bits_for(level_str.size() - 1);
                new_level_str.width(new_level_str_width);

                // Preallocate vector for the worst-case number of blocks
                new_level_str.reserve(level_str.size() / 2 + 1, new_level_str_width);

                {
                    auto block_grid = ctx.split_into_blocks(level_str);

                    dynamic_bit_view_t s = level_str;
                    block_grid.for_each_block_len([&](size_t block_len) {
                        auto slice = s.slice(0, block_len);
                        s = s.slice(block_len);
                        auto rule_name = level.gr.add(slice) - (level.gr.initial_counter() - 1);

                        auto old_cap = new_level_str.capacity();
                        new_level_str.push_back(rule_name);
                        auto new_cap = new_level_str.capacity();
                        DCHECK_EQ(old_cap, new_cap);
                    });
                }

                // Delete previous string
                level.string = dynamic_bit_vector_t();

                DCHECK_EQ(level.string.size(), 0);
                DCHECK_EQ(level.string.capacity(), 0);

                new_level_str.shrink_to_fit();

                // Append to slp array
                {
                    size_t old_slp_size = slp.rules.size();
                    size_t additional_slp_size = level.gr.rules_count();
                    size_t new_slp_size = old_slp_size + additional_slp_size;

                    slp.rules.reserve(new_slp_size);
                    slp.rules.resize(new_slp_size);

                    auto& rv = slp.rules;

                    level.gr.for_all([&](const auto& k, const auto& val_) {
                        const auto& val = val_ - level.gr.initial_counter();
                        const auto& key = k.as_view();

                        size_t store_idx = slp_counter + val - 256;
                        rv[store_idx][0] = key[0] + prev_slp_counter;
                        rv[store_idx][1] = key[1] + prev_slp_counter;
                    });

                    prev_slp_counter = slp_counter;
                    slp_counter += additional_slp_size;
                }

                // carry over stats
                auto level_ipd_stats = level.gr.stats();
                ipd_stats.ext_size2_total += level_ipd_stats.ext_size2_total;
                ipd_stats.ext_size3_total += level_ipd_stats.ext_size3_total;
                ipd_stats.ext_size3_unique += level_ipd_stats.ext_size3_unique;
                ipd_stats.int_size2_total += level_ipd_stats.int_size2_total;
                ipd_stats.int_size2_unique += level_ipd_stats.int_size2_unique;

                // Delete previous hashmap
                level.gr.clear();

                // Prepare next level
                auto tmp = Level<ipd_t> {
                    GrammarRules<ipd_t>(level.gr.rules_count()),
                    level.gr.rules_count(),
                    std::move(new_level_str),
                };

                level_ptr.reset(); // Reset unique pointer to drop contained Level as soon as possible
                level_ptr = std::make_unique<Level<ipd_t>>(std::move(tmp));

                phase.log_stat("SLP size", slp.rules.size());
                phase.log_stat("ext_size2_total", level_ipd_stats.ext_size2_total);
                phase.log_stat("ext_size3_total", level_ipd_stats.ext_size3_total);
                phase.log_stat("ext_size3_unique", level_ipd_stats.ext_size3_unique);
                phase.log_stat("int_size2_total", level_ipd_stats.int_size2_total);
                phase.log_stat("int_size2_unique", level_ipd_stats.int_size2_unique);
            }

            slp.empty = empty;
            slp.root_rule = root_node;

            return slp;
        }
    };
}}
