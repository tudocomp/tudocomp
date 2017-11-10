#pragma once

#include <tudocomp/compressors/esp/FixedVector.hpp>

namespace tdc {namespace esp {
    bool needs_merge(const TypedBlock& a) {
        return a.len == 1;
    };

    bool needs_merge(const TypedBlock& a, const TypedBlock& b) {
        return a.len == 1 || b.len == 1;
    };

    size_t merge(TypedBlock& a, TypedBlock& b, size_t type) {
        if (a.len + b.len == 2) {
            a.len = 2;
            b.len = 2;
            a.type = type;
            b.type = type;
            return 1;
        } else if (a.len + b.len == 3) {
            a.len = 3;
            b.len = 3;
            a.type = type;
            b.type = type;
            return 1;
        } else if (a.len + b.len == 4) {
            a.len = 2;
            b.len = 2;
            a.type = type;
            b.type = type;
            return 2;
        } else {
            DCHECK(false) << "should never happen";
            return 0;
        }
    };

    void adjust_blocks(std::vector<TypedBlock>& blocks) {
        if (blocks.size() < 2) return;

        auto adjust2 = [&](auto f) {
            FixedVector<TypedBlock, 3> queue;
            auto read_it = blocks.begin();
            auto write_it = blocks.begin();

            auto fill = [&]() {
                while ((!queue.full()) && (read_it != blocks.end())) {
                    queue.push_back(std::move(*read_it));
                    ++read_it;
                }
            };

            fill();
            while(queue.view().size() > 0) {
                // std::cout << "Adjust loop:\n";
                // std::cout << "    before:    ";
                // nice_block_lengths(queue.view(), std::cout) << "\n";
                do {
                    fill();
                    //std::cout << "    loop fill: ";
                    //nice_block_lengths(queue.view(), std::cout) << "\n";
                } while(f(queue));
                //std::cout << "    after:     ";
                //nice_block_lengths(queue.view(), std::cout) << "\n";

                auto e = queue.pop_front();
                *write_it = e;
                ++write_it;
            }

            size_t diff = read_it - write_it;
            for (size_t i = 0; i < diff; i++) {
                blocks.pop_back();
            }
        };

        adjust2([](auto& vec) {
            auto v = vec.view();

            bool has_one = false;
            for (auto& e : v) {
                if (e.len == 1) {
                    has_one = true;
                }
            }

            if (!has_one) return false;

            if (v.size() == 3) {
                auto& a = v[1];
                auto& b = v[2];

                if (needs_merge(a, b) && a.type == 2 && b.type == 2) {
                    if (merge(a, b, 2) == 1) {
                        vec.pop_back();
                    }
                    return true;
                }
            }
            if (v.size() >= 2) {
                auto& a = v[0];
                auto& b = v[1];

                if (needs_merge(a, b) && a.type == 2 && b.type == 2) {
                    if (merge(a, b, 2) == 1) {
                        vec.pop_front();
                    }
                    return true;
                }
                if (needs_merge(a, b) && a.type == 3) {
                    if (merge(a, b, 3) == 1) {
                        vec.pop_front();
                    }
                    return true;
                }
                if (needs_merge(a, b) && (a.type == 1 || b.type == 1)) {
                    if (merge(a, b, 1) == 1) {
                        vec.pop_front();
                    }
                    return true;
                }
            }

            // We did all we could do do far
            if (v.size() > 0 && v[0].len > 1) {
                return false;
            }

            DCHECK(false) << "should not be reached";
            return true;

        });
    }
}}
