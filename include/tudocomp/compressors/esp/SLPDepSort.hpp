#pragma once

#include<tudocomp/compressors/esp/SLP.hpp>
#include <queue>

namespace tdc {namespace esp {
    inline void slp_dep_sort(SLP& slp) {
        std::vector<size_t> first_child;
        first_child.reserve(slp.rules.size() + esp::GRAMMAR_PD_ELLIDED_PREFIX);
        first_child.resize(slp.rules.size() + esp::GRAMMAR_PD_ELLIDED_PREFIX);
        for (size_t i = 0; i < first_child.size(); i++) {
            first_child[i] = i;
        }

        std::vector<size_t> next_child;
        next_child.reserve(slp.rules.size() + esp::GRAMMAR_PD_ELLIDED_PREFIX);
        next_child.resize(slp.rules.size() + esp::GRAMMAR_PD_ELLIDED_PREFIX);
        for (size_t i = 0; i < next_child.size(); i++) {
            next_child[i] = i;
        }

        auto append = [&](size_t i, size_t node) {
            DCHECK_NE(i, node);
            if (first_child[i] == i) {
                first_child[i] = node;
            } else {
                auto old_next = first_child[i];
                first_child[i] = node;
                next_child[node] = old_next;
            }
        };

        auto for_all = [&](size_t i, auto f) {
            size_t l = first_child[i];
            if (l != i) {
                f(l);
                while (true) {
                    if (next_child[l] != l) {
                        l = next_child[l];
                    } else {
                        break;
                    }
                    f(l);
                }
            }
        };

        for (size_t i = 0; i < slp.rules.size(); i++) {
            auto j = slp.rules.size() - i - 1;
            {
                auto node = j + esp::GRAMMAR_PD_ELLIDED_PREFIX;
                auto left_child = slp.rules[j][0];
                append(left_child, node);
            }
        }

        /*for (size_t i = 0; i < inv.size(); i++) {
            if (i < 32 || i > 127) {
                std::cout << i       << " -> ";
            } else {
                std::cout << char(i) << " -> ";
            }
            std::cout << vec_to_debug_string(inv[i]);
            {
                std::vector<size_t> tmp;
                for_all(i, [&](size_t l) {
                    tmp.push_back(l);
                });

                std::cout << "; " << vec_to_debug_string(tmp);
            }
            std::cout << "\n";
        }*/

        std::vector<size_t> rename;
        rename.reserve(slp.rules.size());
        rename.resize(slp.rules.size());

        std::queue<size_t> queue;
        for (size_t i = 0; i < 256; i++) {
            queue.push(i);
        }
        size_t counter = 0;

        while (!queue.empty()) {
            auto elem = queue.front();
            queue.pop();
            for_all(elem, [&](size_t child) {
                queue.push(child);
            });

            auto new_value = counter++;

            if (elem < 256) {
                DCHECK_EQ(elem, new_value);
            } else {
                auto original_idx = elem - esp::GRAMMAR_PD_ELLIDED_PREFIX;
                auto new_idx      = new_value - esp::GRAMMAR_PD_ELLIDED_PREFIX;

                rename[original_idx] = new_idx;
            }

            //std::cout << elem << ": " << new_value << "\n";
        }

        {
            auto discard = std::move(first_child);
            discard = std::move(next_child);
        }

        // TODO: Could save one n alloc here by renaming DL and DR after each other
        std::vector<std::array<size_t, 2>> renamed_slp;
        renamed_slp.reserve(slp.rules.size());
        renamed_slp.resize(slp.rules.size());

        for (size_t i = 0; i < renamed_slp.size(); i++) {
            renamed_slp[rename[i]] = slp.rules[i];
            for (auto& e : renamed_slp[rename[i]]) {
                if (e > 255) {
                    e = rename[e - esp::GRAMMAR_PD_ELLIDED_PREFIX] + esp::GRAMMAR_PD_ELLIDED_PREFIX;
                }
            }
        }

        slp.rules = std::move(renamed_slp);
        if (slp.root_rule > 255) {
            slp.root_rule = rename[slp.root_rule - esp::GRAMMAR_PD_ELLIDED_PREFIX] + esp::GRAMMAR_PD_ELLIDED_PREFIX;
        }
    }
}}
