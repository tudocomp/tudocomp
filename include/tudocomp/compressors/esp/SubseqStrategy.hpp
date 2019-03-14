#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/esp/SLP.hpp>
#include <tudocomp/compressors/esp/MonotoneSubsequences.hpp>

namespace tdc {namespace esp {
    class SubSeqOptimal: public Algorithm {
    public:
        inline static Meta meta() {
            Meta m(TypeDesc("subseq"), "optimal");
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
            Meta m(TypeDesc("subseq"), "greedy");
            return m;
        };

        using Algorithm::Algorithm;

        template<typename SortedIndices>
        inline Dpi_and_b create_dpi_and_b_from_sorted_indices(const SortedIndices& sis) const {
            Dpi_and_b ret;
            ret.Dpi.reserve(sis.size());
            ret.Dpi.resize(sis.size(), 999);

            size_t Dpi_counter = 0;

            DCHECK_GE(sis.size(), 1U);
            std::vector<std::pair<size_t, size_t>> free_list;
            free_list.reserve(sis.size() + 2);
            free_list.resize(sis.size() + 2);
            for (size_t i = 1; i < (free_list.size() - 3); i++) {
                free_list[i].first = i - 1;
                free_list[i].second = i + 1;
            }
            const size_t start_dummy_link = free_list.size() - 2;
            const size_t end_dummy_link   = free_list.size() - 1;

            if ((free_list.size() - 2) > 1) {
                free_list[0].first = start_dummy_link;
                free_list[0].second = 1;
                free_list[free_list.size() - 3].first = free_list.size() - 4;
                free_list[free_list.size() - 3].second = end_dummy_link;
            } else {
                free_list[0].first = start_dummy_link;
                free_list[0].second = end_dummy_link;
            }

            free_list[start_dummy_link].first = start_dummy_link;
            free_list[end_dummy_link].second = end_dummy_link;

            free_list[start_dummy_link].second = 0;
            free_list[end_dummy_link].first = free_list.size() - 3;

            std::vector<size_t> increasing;
            std::vector<size_t> decreasing;
            //std::cout << "sis:\n";
            //std::cout << vec_to_debug_string(sis, 3) << "\n\n";

            auto si_of = [&](size_t link) {
                DCHECK(link != start_dummy_link);
                DCHECK(link != end_dummy_link);
                return sis[link];
            };

            auto remove = [&](size_t link) {
                DCHECK(link != start_dummy_link);
                DCHECK(link != end_dummy_link);
                auto prev = free_list[link].first;
                auto next = free_list[link].second;

                free_list[prev].second = next;
                free_list[next].first = prev;

                free_list[link].first = link;
                free_list[link].second = link;
            };

            auto handle = [&](const std::vector<size_t>& seq) {
                for (auto link : seq) {
                    DCHECK(link != start_dummy_link);
                    DCHECK(link != end_dummy_link);
                    ret.Dpi[link] = Dpi_counter;
                }

                for (auto link : seq) {
                    remove(link);
                }

                Dpi_counter++;
            };

            while (free_list[start_dummy_link].second != end_dummy_link) {
                {
                    size_t cstart = free_list[start_dummy_link].second;
                    increasing.clear();
                    increasing.push_back(cstart);
                    while(true) {


                        if (free_list[cstart].second == end_dummy_link) {
                            //std::cout << "inc done\n";
                            break;
                        } else {
                            //std::cout << cstart << "\n";
                            cstart = free_list[cstart].second;
                            //std::cout << cstart << "\n";
                        }

                        DCHECK(si_of(cstart) != si_of(increasing.back()));
                        if (si_of(cstart) > si_of(increasing.back())) {
                            increasing.push_back(cstart);
                        }
                    }
                }
                {
                    size_t cend = free_list[end_dummy_link].first;
                    decreasing.clear();
                    decreasing.push_back(cend);
                    while(true) {

                        if (free_list[cend].first == start_dummy_link) {
                            //std::cout << "dec done\n";
                            break;
                        } else {
                            cend = free_list[cend].first;
                        }

                        DCHECK(si_of(cend) != si_of(decreasing.back()));
                        if (si_of(cend) > si_of(decreasing.back())) {
                            decreasing.push_back(cend);
                        }
                    }
                    // TODO: Just for debug
                    std::reverse(decreasing.begin(), decreasing.end());
                }

                //debug(increasing);
                //debug(decreasing);

                if (increasing.size() >= decreasing.size()) {
                    handle(increasing);
                    ret.b.push_back(uint_t<1>(0));
                    //std::cout << "-> "; debug(increasing);
                } else {
                    handle(decreasing);
                    ret.b.push_back(uint_t<1>(1));
                    //std::cout << "-> "; debug(decreasing);
                }
                //std::cout << vec_to_debug_string(ret.Dpi, 3) << "\n";
                //std::cout << vec_to_debug_string(ret.b) << "\n\n";
                //std::cout << "\n";
            }
            //std::cout << "\n";

            return ret;
        }
    };
}}
