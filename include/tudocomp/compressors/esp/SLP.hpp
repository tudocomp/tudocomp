#pragma once

#include <memory>
#include <array>

namespace tdc {namespace esp {
    static_assert(sizeof(std::array<size_t, 2>) == sizeof(size_t) * 2, "Something is not right");

    // TODO: Fix to work with = 0
    size_t GRAMMAR_PD_ELLIDED_PREFIX = 256;

    struct SLP {
        std::vector<std::array<size_t, 2>> rules;
        size_t root_rule;
        bool empty = false;

        inline void derive_text_rec(std::ostream& o, size_t rule) const {
            if (rule < 256) {
                o << char(rule);
            } else for (auto r : rules[rule - GRAMMAR_PD_ELLIDED_PREFIX]) {
                derive_text_rec(o, r);
            }
        }

        inline std::ostream& derive_text(std::ostream& o) const {
            derive_text_rec(o, root_rule);
            return o;
        }

        inline std::string derive_text_s() const {
            std::stringstream ss;
            derive_text(ss);
            return ss.str();
        }
    };
}}
