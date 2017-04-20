#pragma once

#include<tudocomp/compressors/esp/SLP.hpp>
#include <queue>

namespace tdc {namespace esp {
    inline void slp_dep_sort(SLP& slp) {
        //inverse_deps(slp, slp.root_rule);

        // inverse
        //std::vector<size_t> inv;
        //inv.reserve(slp.rules.size() + esp::GRAMMAR_PD_ELLIDED_PREFIX);
        //inv.resize(slp.rules.size() + esp::GRAMMAR_PD_ELLIDED_PREFIX, -1);
        //inverse_deps(slp, slp.root_rule, inv);
        std::vector<std::vector<size_t>> inv;
        inv.reserve(slp.rules.size() + esp::GRAMMAR_PD_ELLIDED_PREFIX);
        inv.resize(slp.rules.size() + esp::GRAMMAR_PD_ELLIDED_PREFIX);

        for (size_t i = 0; i < slp.rules.size(); i++) {
            auto node = i + esp::GRAMMAR_PD_ELLIDED_PREFIX;
            auto left_child = slp.rules[i][0];
                // + esp::GRAMMAR_PD_ELLIDED_PREFIX;

            auto& v = inv[left_child];
            v.push_back(node);
        }

        for (size_t i = 0; i < inv.size(); i++) {
            if (i < 32 || i > 127) {
                //std::cout << i       << " -> " << vec_to_debug_string(inv[i]) << "\n";
            } else {
                //std::cout << char(i) << " -> " << vec_to_debug_string(inv[i]) << "\n";
            }
        }

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
            for (auto child : inv[elem]) {
                queue.push(child);
            }

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
