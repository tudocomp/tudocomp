#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/ds/IntVector.hpp>

#include <tudocomp/util/View.hpp>
#include <tudocomp/compressors/esp/pre_header.hpp>
#include <tudocomp/compressors/esp/meta_blocks.hpp>

namespace tdc {
namespace esp {
    using in_t = ConstGenericView<size_t>;

    template<size_t N>
    struct Array {
        std::array<size_t, N> m_data;
        Array() {
            for(size_t i = 0; i < N; i++) {
                m_data[i] = 0;
            }
        }
        Array(in_t v) {
            DCHECK_EQ(v.size(), N);
            for(size_t i = 0; i < N; i++) {
                m_data[i] = v[i];
            }
        }
    };

    template<size_t N>
    bool operator==(const Array<N>& lhs, const Array<N>& rhs) {
        for(size_t i = 0; i < N; i++) {
            if (lhs.m_data[i] != rhs.m_data[i]) return false;
        }
        return true;
    }
}
}
namespace std {
    template<size_t N>
    struct hash<tdc::esp::Array<N>>
    {
        size_t operator()(const tdc::esp::Array<N>& x) const {
            return std::hash<tdc::ConstGenericView<size_t>>()(
                tdc::ConstGenericView<size_t> {
                    x.m_data.data(),
                    x.m_data.size()
                });
        }
    };
}

namespace tdc {
namespace esp {

    struct GrammarRules {
        GrammarRules() {}
        using a2_t = std::unordered_map<Array<2>, size_t>;
        using a3_t = std::unordered_map<Array<3>, size_t>;

        a2_t n2;
        a3_t n3;

        size_t counter = 1;

        size_t add(in_t v) {
            size_t* r;
            if (v.size() == 2) {
                r = &n2[v];
            } else if (v.size() == 3) {
                r = &n3[v];
            } else {
                DCHECK(false);
            }

            if (*r == 0) {
                *r = counter++;
            }

            return *r - 1;
        }
    };

    struct Round {
        GrammarRules gr;
        size_t alphabet;
        std::vector<size_t> string;
    };

    std::vector<Round> generate_grammar_rounds(string_ref input,
                                               bool silent = false) {
        std::vector<Round> rounds;

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

            if (r.string.size() == 1) {
                if (!silent) std::cout << "Done\n";
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

        return std::move(rounds);
    }

    ///
    /// REFACTOR: ONE GLOBAL HASHMAP STRUCTURE WITH GLOBAL ARRAY INDICE CONTENTS
    /// SUBTRACT DOWN TO LAYER_INDEX MANUALLY WHERE NEEDED
    ///
    /// NEW GOAL
    ///
    /// direct hashmap -> slp conversion, with push-to-end and
    /// remembering original start symbol

    size_t GRAMMAR_PD_ELLIDED_PREFIX = 256;

    struct SLP {
        std::vector<std::array<size_t, 2>> rules;
        size_t root_rule;
    };

    // TODO: Either ellided-256 alphabet, or compressed included alphabet

    SLP generate_grammar(const std::vector<Round>& rs) {
        size_t size2 = 0;
        size_t size3 = 0;
        for (auto& r: rs) {
            size2 += r.gr.n2.size();
            size3 += r.gr.n3.size();
        }

        std::cout << "size2: " << size2 << "\n";
        std::cout << "size3: " << size3 << "\n";

        std::vector<std::array<size_t, 2>> ret;
        ret.reserve(size2 + 2 * size3);
        ret.resize(size2 + 2 * size3, std::array<size_t, 2>{{ 0,0 }});

        size_t i3 = size2 + size3;

        size_t counter_offset = 256;
        size_t prev_counter_offset = 0;

        size_t last_idx = 0;

        size_t debug_round = 0;
        for (auto& r: rs) {
            auto doit = [&](auto& n) {
                for (auto& kv: n) {
                    const auto& key = kv.first;
                    const auto& val = kv.second - 1;

                    size_t rule_idx = counter_offset + val;
                    size_t store_idx = rule_idx - 256;

                    last_idx = rule_idx;

                    std::cout << "round(" << debug_round << "): "
                        << "rule_idx: " << rule_idx
                        << ", store_idx: " << store_idx
                        << "\n";

                    DCHECK_EQ(ret.at(store_idx).at(0), 0);
                    DCHECK_EQ(ret.at(store_idx).at(1), 0);

                    if (key.m_data.size() == 2) {
                        std::cout << "store to 2 idx " << store_idx << "\n";

                        ret[store_idx][0] = key.m_data[0] + prev_counter_offset;
                        ret[store_idx][1] = key.m_data[1] + prev_counter_offset;
                    } else if (key.m_data.size() == 3) {
                        std::cout << "store to 2 idx " << store_idx << "\n";
                        ret[store_idx][0] = i3 + 256;
                        ret[store_idx][1] = key.m_data[2] + prev_counter_offset;

                        std::cout << "store to 3 idx " << i3 << "\n";
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

        DCHECK_EQ(counter_offset, size2 + size3 + 256);
        DCHECK_EQ(i3, ret.size());

        return SLP {
            std::move(ret),
            last_idx
        };
    }

    void derive_text_rec(const SLP& slp,
                         std::ostream& o,
                         size_t rule) {
        if (rule < 256) {
            o << char(rule);
        } else for (auto r : slp.rules[rule - 256]) {
            derive_text_rec(slp, o, r);
        }
    }

    std::string derive_text(const SLP& slp) {
        std::stringstream ss;

        derive_text_rec(slp, ss, slp.root_rule);

        return ss.str();
    }

}
}
