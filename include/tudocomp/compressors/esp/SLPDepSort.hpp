#pragma once

#include<tudocomp/compressors/esp/SLP.hpp>
#include <queue>

namespace tdc {namespace esp {
    inline void slp_dep_sort(SLP& slp) {
        // TODO: Use bitvectors here

        size_t const bit_width = slp.width();
        size_t const slp_size = slp.size();

        DCHECK_EQ(bit_width, 64);

        DynamicIntVector first_child;
        first_child.width(bit_width);
        first_child.reserve(slp_size);
        first_child.resize(slp_size);
        for (size_t i = 0; i < slp_size; i++) {
            first_child[i] = i;
        }

        DynamicIntVector next_child;
        next_child.width(bit_width);
        next_child.reserve(slp_size);
        next_child.resize(slp_size);
        for (size_t i = 0; i < slp_size; i++) {
            next_child[i] = i;
        }

        auto append = [&](size_t i, size_t node) {
            DCHECK_NE(i, node);
            if (first_child[i] == i) {
                first_child[i] = node;
            } else {
                size_t old_next = first_child[i];
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

        for (size_t i = slp_size; i > SLP_CODING_ALPHABET_SIZE; i--) {
            auto node = i - 1;
            append(slp.get_l(node), node);
        }

        DynamicIntVector rename;
        rename.width(bit_width);
        rename.reserve(slp_size);
        rename.resize(slp_size);

        std::queue<size_t> queue;
        for (size_t i = 0; i < SLP_CODING_ALPHABET_SIZE; i++) {
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

            if (elem < SLP_CODING_ALPHABET_SIZE) {
                DCHECK_EQ(elem, new_value);
            }

            rename[elem] = new_value;

            //std::cout << elem << ": " << new_value << "\n";
        }

        {
            auto _discard = std::move(next_child);
        }

        auto tmp = std::move(first_child);
        auto& dl = slp.dl();
        auto& dr = slp.dr();

        // Rename DL
        for (size_t i = 0; i < slp_size; i++) {
            tmp[rename[i]] = rename[dl[i]];
        }
        swap(tmp, dl);

        // Rename DR
        for (size_t i = 0; i < slp_size; i++) {
            tmp[rename[i]] = rename[dr[i]];
        }
        swap(tmp, dr);

        // Rename root node
        slp.set_root_rule(rename[slp.root_rule()]);
    }
}}
