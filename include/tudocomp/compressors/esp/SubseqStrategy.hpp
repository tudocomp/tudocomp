#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/esp/SLP.hpp>
#include <tudocomp/compressors/esp/MonotoneSubsequences.hpp>

namespace tdc {namespace esp {
    class SubSeqOptimal: public Algorithm {
    public:
        inline static Meta meta() {
            Meta m("subseq", "optimal");
            return m;
        };

        using Algorithm::Algorithm;

        template<typename SortedIndices>
        inline Dpi_and_b create_dpi_and_b_from_sorted_indices(const SortedIndices& sorted_indices) const {
            return esp::create_dpi_and_b_from_sorted_indices(sorted_indices);
        }
    };
    class SubSeqGreedy: public Algorithm {
    public:
        inline static Meta meta() {
            Meta m("subseq", "greedy");
            return m;
        };

        using Algorithm::Algorithm;

        template<typename SortedIndices>
        inline Dpi_and_b create_dpi_and_b_from_sorted_indices(const SortedIndices& sis) const {
            Dpi_and_b ret;
            ret.Dpi.reserve(sis.size());
            ret.Dpi.resize(sis.size());

            size_t Dpi_counter = 0;

            std::vector<size_t> free_list; // one dummy node at the end
            free_list.reserve(sis.size() + 1);
            free_list.resize(sis.size() + 1);
            for (size_t i = 0; i < (free_list.size() - 1); i++) {
                free_list[i] = i + 1;
            }
            free_list[free_list.size() - 2] = free_list.size() - 2;

            const size_t dummy_link = free_list.size() - 1;
            free_list[dummy_link] = 0;

            std::vector<size_t> increasing;
            std::vector<size_t> decreasing;
            std::cout << "sis:\n";
            std::cout << vec_to_debug_string(sis) << "\n";
            while (free_list[dummy_link] != dummy_link) {
                size_t current = dummy_link;
                increasing.clear();
                decreasing.clear();
                increasing.push_back(current);
                decreasing.push_back(current);

                auto si_of = [&](size_t link) {
                    return sis[free_list[link]];
                };

                auto debug = [&](const auto& links) {
                    std::vector<size_t> x;
                    for (auto y : links) {
                        x.push_back(si_of(y));
                    }
                    std::cout << vec_to_debug_string(x) << "\n";
                };

                while(true) {
                    debug(increasing);
                    debug(decreasing);
                    std::cout << "\n";

                    {
                        auto next = free_list[current];
                        if (next == current) {
                            std::cout << "next\n";
                            break;
                        } else {
                            current = next;
                        }
                    }

                    DCHECK(si_of(current) != si_of(increasing.back()));
                    DCHECK(si_of(current) != si_of(decreasing.back()));

                    if (si_of(current) > si_of(increasing.back())) {
                        increasing.push_back(current);
                    } else if (si_of(current) < si_of(decreasing.back())) {
                        decreasing.push_back(current);
                    }
                }

                auto remove_next = [&](size_t index) {
                    auto prev = index;
                    auto link = free_list[prev];
                    auto next = free_list[link];

                    if (next != link) {
                        free_list[prev] = next;
                    } else {
                        free_list[prev] = prev;
                    }
                };

                auto handle = [&](const std::vector<size_t>& seq) {
                    for (auto link : seq) {
                        ret.Dpi[free_list[link]] = Dpi_counter;
                    }

                    for (auto link : seq) {
                        remove_next(link);
                    }

                    Dpi_counter++;
                };

                if (increasing.size() >= decreasing.size()) {
                    handle(increasing);
                    ret.b.push_back(uint_t<1>(0));
                } else {
                    handle(decreasing);
                    ret.b.push_back(uint_t<1>(1));
                }
            }

            return ret;
        }
    };
}}
