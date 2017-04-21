#pragma once

#include <tudocomp/ds/IntVector.hpp>

namespace tdc {namespace esp {
    using Sindex = size_t;
    using Link = size_t;

    inline std::vector<Sindex> sorted_indices(ConstGenericView<size_t> input) {
        std::vector<Sindex> sorted_indices;
        sorted_indices.reserve(input.size());
        for(size_t i = 0; i < input.size(); i++) {
            sorted_indices.push_back(i);
        }
        DCHECK_EQ(sorted_indices.capacity(), input.size());

        // TODO: Measure memory, might try to check unstable sort here
        std::stable_sort(sorted_indices.begin(), sorted_indices.end(), [&](const size_t& a, const size_t& b) {
            return input[a] < input[b];
        });

        return sorted_indices;
    }

    struct Point {
        size_t x;
        size_t y;
    };

    inline bool operator==(const Point& a, const Point& b) {
        return a.x == b.x && a.y == b.y;
    }

    inline std::ostream& operator<<(std::ostream& o, const Point& a) {
        return o << "(" << a.x << ", " << a.y << ")";
    }

    inline Point point_coord_if_at_index(Sindex self, size_t index) {
        return Point {
            index,
            self
        };
    }

    class LayersIterator {
        // yields addresses in descending order of in normal mode,
        // and in ascending order if in reverse mode.
        bool m_reverse;
        size_t m_front;
        size_t m_back;
        IntVector<uint_t<1>>* m_skip;

        inline void debug() {
            std::cout << "front: " << m_front << ", back: " << m_back << ", bits: ";
            std::cout << vec_to_debug_string(*m_skip) << "\n";
        }

        inline void front_skip() {
            //std::cout << "fs: "; debug();
            auto& skip = *m_skip;
            while (m_front != m_back && m_front < skip.size() && skip[m_front] == uint_t<1>(1)) {
                m_front++;
            }
        }
        inline void back_skip() {
            //std::cout << "bs: "; debug();
            auto& skip = *m_skip;
            while (m_front != m_back && m_back > 0 && skip[m_back - 1] == uint_t<1>(1)) {
                m_back--;
            }
        }
    public:
        inline LayersIterator(IntVector<uint_t<1>>& skip, bool reverse):
            m_reverse(reverse),
            m_front(0),
            m_back(skip.size()),
            m_skip(&skip)
        {
            front_skip();
            back_skip();
        }

        inline bool has_next() {
            //std::cout << "hn: "; debug();
            return m_front != m_back;
        }

        inline Link advance() {
            //std::cout << "av: "; debug();
            size_t r;
            if (!m_reverse) {
                r = --m_back;
                back_skip();
            } else {
                r = m_front++;
                front_skip();
            }
            //std::cout << "\n";
            return r;
        }
    };

    inline size_t layers_A_search(ConstGenericView<size_t> searchA, size_t piy) {
        // TODO: Implement binary search here
        size_t j = 0;
        for(size_t i = 0; i < searchA.size(); i++) {
            if (searchA[i] > piy) {
                j = i;
            }
        }
        return j;
    }

    inline Point point_coord_for_link(ConstGenericView<Sindex> sindices, Link link, bool reverse) {
        size_t inverted_link;
        if (!reverse) {
            inverted_link = link;
        } else {
            inverted_link = sindices.size() - link - 1;
        }

        return point_coord_if_at_index(sindices[link], inverted_link);
    }

    class L {
        ConstGenericView<Sindex> m_sindices;
        std::vector<Link> m_linked_list;
        std::vector<Link> m_layers;
        bool m_reverse = false;
        std::vector<size_t> m_rebuild_A; // allocation cache
        IntVector<uint_t<1>> m_removed;
        size_t m_removed_counter = 0;
        size_t m_remaining_size;
    public:
        inline L(ConstGenericView<Sindex> sindices) {
            m_linked_list.reserve(sindices.size());
            m_linked_list.resize(sindices.size());
            m_sindices = sindices;
            m_removed.reserve(sindices.size());
            m_removed.resize(sindices.size());
            m_remaining_size = sindices.size();
        }

        void rebuild(bool reverse) {
            m_layers.clear();
            m_reverse = reverse;
            auto& A = m_rebuild_A;
            A.clear();
            A.push_back(size_t(-1));
            size_t l = 0;

            auto layers_iter = LayersIterator(m_removed, m_reverse);

            while (layers_iter.has_next()) {
                auto link = layers_iter.advance();
                auto pi = point_coord_for_link(m_sindices, link, m_reverse);
                auto j = layers_A_search(ConstGenericView<size_t>(A).slice(0, l + 1), pi.y);
                auto is_new_layer = (j == l);
                if (is_new_layer) {
                    m_layers.push_back(Link());
                    l += 1;
                }

                auto& Lj_plus = m_layers[j];

                // set next pointer in linked list to null
                m_linked_list[link] = link;
                if (is_new_layer) {
                    // Just make the layer point to the first element of the linked list
                    Lj_plus = link;
                } else {
                    // Append link before the first element of the linked list
                    auto old_head = Lj_plus;
                    m_linked_list[link] = old_head;
                    Lj_plus = link;
                }

                // Grow A as needed
                if (A.size() == (j + 1)) {
                    A.push_back(pi.y);
                } else {
                    A[j + 1] = pi.y;
                }
            }
        }

        inline bool dominates(Link lhs, Link rhs) {
            auto lhsp = point_coord_for_link(m_sindices, lhs, m_reverse);
            auto rhsp = point_coord_for_link(m_sindices, rhs, m_reverse);

            return (lhsp.x > rhsp.x) && (lhsp.y > rhsp.y);
        }

        // return false if error
        // q is a out parameter
        inline bool lis(size_t k, std::vector<size_t>& q) {
            if (k > m_layers.size()) { return false; }
            q.clear();
            q.reserve(k);
            q.resize(k);
            if (k == 0) { return true; }

            size_t l = k - 1;

            q[l] = m_layers[l];

            while (true) {
                if (l == 0) {
                    break;
                } else {
                    l -= 1;
                }

                Link search_link = m_layers[l];

                while (true) {
                    if (dominates(search_link, q[l + 1])) {
                        q[l] = search_link;
                        break;
                    } else {
                        DCHECK_NE(m_linked_list[search_link], search_link)
                            << "should always break out before this happens";
                        search_link = m_linked_list[search_link];
                    }
                }
            }

            // TODO: test if it gets faster by not reversing unneeded
            std::reverse(q.begin(), q.end());

            return true;
        }

        inline void remove_all_and_slice(ConstGenericView<size_t> links) {
            // m_removed

            // Mark the links as deleted
            for(auto link : links) {
                m_removed[link] = uint_t<1>(1);
                m_linked_list[link] = m_removed_counter;
            }

            m_removed_counter++;
            m_layers.clear();
            m_remaining_size -= links.size();
        }

        inline std::vector<std::vector<Point>> to_debug_layer_points() {
            std::vector<std::vector<Point>> r;
            for (auto link : m_layers) {
                std::vector<Point> list;

                while (true) {
                    list.push_back(point_coord_for_link(m_sindices, link, m_reverse));
                    if (m_linked_list[link] == link) {
                        break;
                    } else {
                        link = m_linked_list[link];
                    }
                }

                r.push_back(std::move(list));
            }
            return r;
        }

        inline size_t layers_size() {
            return m_layers.size();
        }

        inline Sindex sindex_for_link(Link link) {
            return m_sindices[link];
        }

        inline size_t sindices_size() {
            return m_remaining_size;
        }

        inline std::vector<size_t> extract() && {
            return std::move(m_linked_list);
        }
    };

    class MonotonSubsequences {
        // TODO: Bitvectors
        std::vector<size_t> m_sorted_indices;

    public:


    };
}}
