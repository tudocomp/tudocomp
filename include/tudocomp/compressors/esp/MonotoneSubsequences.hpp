#pragma once

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
    public:
        inline LayersIterator(size_t sindex_len, bool reverse):
            m_reverse(reverse),
            m_front(0),
            m_back(sindex_len) {
        }

        inline bool has_next() {
            return m_front != m_back;
        }

        inline Link advance() {
            if (!m_reverse) {
                m_back--;
                return m_back;
            } else {
                auto tmp = m_front;
                m_front++;
                return tmp;
            }
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
        GenericView<Sindex> m_sindices;
        std::vector<Link> m_linked_list;
        std::vector<Link> m_layers;
        bool m_reverse = false;
        std::vector<size_t> m_rebuild_A; // allocation cache
    public:
        inline L(GenericView<Sindex> sindices) {
            m_linked_list.reserve(sindices.size());
            m_linked_list.resize(sindices.size());
            m_sindices = sindices;
        }

        void rebuild(bool reverse) {
            m_layers.clear();
            m_reverse = reverse;
            auto& A = m_rebuild_A;
            A.clear();
            A.push_back(size_t(-1));
            size_t l = 0;

            auto layers_iter = LayersIterator(m_sindices.size(), m_reverse);

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

            auto l = k - 1;
            while (true) {
                if (l == 0) {
                    break;
                } else {
                    l -= 1;
                }

                auto search_link = m_layers[l];

                while (true) {
                    if (dominates(search_link, q[l + 1])) {
                        q[l] = search_link;
                        break;
                    } else {
                        DCHECK_NE(m_linked_list[search_link], search_link);
                        search_link = m_linked_list[search_link];
                    }
                }
            }

            // TODO: test if it gets faster by not reversing unneeded
            std::reverse(q.begin(), q.end());
        }

        inline void remove_all_and_slice(ConstGenericView<size_t> links) {
            // Save the original indices in the now vacant linked list slots
            for(auto link : links) {
                m_linked_list[link] = m_sindices[link];
            }

            // Check which way we need to read the links array
            auto is_reverse = links.size() > 1 && links[0] > links[1];

            // Remove the m_sindices entries
            {
                size_t links_i = 0;
                size_t sii_r = 0;
                size_t sii_w = 0;

                // iterate over all sindices, skipping removed ones
                while (sii_r < m_sindices.size()) {
                    if (links_i < links.size()) {
                        size_t adjusted_links_i;
                        if (is_reverse) {
                            adjusted_links_i = links.size() - links_i - 1;
                        } else {
                            adjusted_links_i = links_i;
                        }
                        if (sii_r == links[adjusted_links_i]) {
                            ++sii_r;
                            ++links_i;
                            continue;
                        }
                    }

                    m_sindices[sii_w] = m_sindices[sii_r];
                    ++sii_r;
                    ++sii_w;
                }
            }

            // Copy sindex values back to the now vacant end of m_sindices
            size_t offset = m_sindices.size() - links.size();
            for(size_t i = 0; i < links.size(); i++) {
                m_sindices[offset + i] = m_linked_list[links[i]];
            }

            // NB: could prepare linked list in a way that preserves
            // addresses across deletion here, but leaving it in a
            // undefined state instead since we rebuild from scratch.

            m_layers.clear();
            m_sindices = m_sindices.slice(0, m_sindices.size() - links.size());
            for (size_t i = 0; i < links.size(); i++) {
                m_linked_list.pop_back();
            }
        }

    };

    class MonotonSubsequences {
        // TODO: Bitvectors
        std::vector<size_t> m_sorted_indices;

    public:


    };
}}
