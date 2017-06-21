#pragma once

#include <memory>
#include <array>

namespace tdc {namespace esp {
    static_assert(sizeof(std::array<size_t, 2>) == sizeof(size_t) * 2, "Something is not right");

    // TODO: Fix to work with = 0
    size_t GRAMMAR_PD_ELLIDED_PREFIX = 256;

    struct SLP {
        std::vector<std::array<size_t, 2>> rules;
        size_t root_rule = 0;
        bool empty = true;

        inline SLP() {}
        inline SLP(std::vector<std::array<size_t, 2>>&& r,
                   size_t root,
                   bool e):
            rules(std::move(r)),
            root_rule(root),
            empty(e) {}

        inline void derive_text_rec(std::ostream& o, size_t rule) const {
            if (rule < 256) {
                o << char(rule);
            } else for (auto r : rules[rule - GRAMMAR_PD_ELLIDED_PREFIX]) {
                derive_text_rec(o, r);
            }
        }

        inline std::ostream& derive_text(std::ostream& o) const {
            if (!empty) {
                derive_text_rec(o, root_rule);
            }
            return o;
        }

        inline std::string derive_text_s() const {
            std::stringstream ss;
            derive_text(ss);
            return ss.str();
        }

        // Returns idx of node n
        inline size_t node_idx(size_t n) const {
            DCHECK_GE(n, GRAMMAR_PD_ELLIDED_PREFIX);
            return n - GRAMMAR_PD_ELLIDED_PREFIX;
        }

        inline const std::array<size_t, 2>& node(size_t n) const {
            return rules.at(node_idx(n));
        }
    };

    struct SLPRhsAdapter {
        using value_type = size_t;

        SLP* slp;
        inline const size_t& operator[](size_t i) const {
            return slp->rules[i][1];
        }
        inline size_t& operator[](size_t i) {
            return slp->rules[i][1];
        }
        inline size_t size() const {
            return slp->rules.size();
        }
    };
}}
