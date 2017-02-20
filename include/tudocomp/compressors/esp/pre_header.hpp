#pragma once

#include <tudocomp/util.hpp>
#include <tudocomp/Env.hpp>
#include <tudocomp/Compressor.hpp>
#include <tudocomp/ds/IntVector.hpp>

namespace tdc {
namespace esp {
    // wether to maximize or minimize repeating meta blocks
    const bool MAXIMIZE_REPEATING = true;

    // Wether to tie landmark-blocks to the right or to the left
    const bool TIE_TO_RIGHT = true;

    /*

    Paramters:
    - treatment of with-1 metablocks
    - repeating metablock large or small
        [aaa|sdfgh|aaa]
        vs [aa|asdfgha|aa}
    - Tests for cases
        [aa|bababa] vs [a|abababa], eg "aabababababababaa"

    */

    using int_vector::IntVector;

    // Implementation that covers all of 64 bit
    // TODO: Does the Paper mean base-e or base-2 ?
    inline size_t iter_log(size_t n) {
        size_t o = 1;
        if (n < 3) return 1 + o;
        if (n < 16) return 2 + o;
        if (n < 3814280) return 3 + o;
        return 4 + o;
    }


    template<class T>
    uint64_t calc_alphabet_size(const T& t) {
        std::bitset<256> bs;
        for (uint8_t v : t) {
            bs.set(v, true);
        }
        return bs.count();
    }

    template<class T>
    bool no_adjacent_identical(const T& t) {
        for(size_t i = 1; i < t.size(); i++) {
            if (t[i] == t[i - 1]) return false;
        }
        return true;
    }

    uint64_t label(uint64_t left, uint64_t right) {
        auto diff = left ^ right;

        //std::cout << "l: " << std::setbase(2) << left << "\n";
        //std::cout << "r: " << std::setbase(2) << right << "\n";
        //std::cout << "d: " << std::setbase(2) << diff << "\n";
        //std::cout << "\n";


        DCHECK(diff != 0);

        auto l = __builtin_ctz(diff);

        auto bit = [](uint8_t l, uint64_t v) {
            // TODO: test
            return (v >> l) & 1;
        };

        // form label(A[i])
        return 2*l + bit(l, right);
    };

    template<class T, class F>
    void for_neigbors(T& t, F f) {
        for (size_t i = 0; i < t.size(); i++) {
            typename T::value_type neighbors[2];
            uint8_t neighbor_len = 0;

            if (i == 0 && i == t.size() - 1) {
                neighbor_len = 0;
            } else if (i == 0) {
                neighbor_len = 1;
                neighbors[0] = t[i + 1];
            } else if (i == t.size() - 1) {
                neighbor_len = 1;
                neighbors[0] = t[i - 1];
            } else {
                neighbor_len = 2;
                neighbors[0] = t[i - 1];
                neighbors[1] = t[i + 1];
            }

            f(i, neighbors, neighbor_len);
        }
    }

    template<class T>
    bool check_landmarks(const T& t, bool allow_long = false) {
        if (allow_long) return true;
        size_t last = 0;
        size_t i = 0;
        for(; i < t.size(); i++) {
            if (t[i] == 1u) {
                if (i > 1) return false;
                last = i;
                i++;
                break;
            }
        }
        for(; i < t.size(); i++) {
            if (t[i] == 1u) {
                if ((i - last) > 3 || (i - last) < 2) return false;
                last = i;
            }
        }
        return true;
    }

    template<typename T>
    class DebugPrint {
        ConstGenericView<T> m_view;
        size_t m_alpha;
    public:
        inline DebugPrint(ConstGenericView<T> v, size_t alpha):
            m_view(v), m_alpha(alpha) {}
        template<typename U>
        friend std::ostream& operator<<(std::ostream&, const DebugPrint<U>&);
        size_t char_mult() {
            if (m_alpha == 256) {
                return 1;
            } else {
                return 4;
            }
        }
    };
    template<typename T>
    inline std::ostream& operator<<(std::ostream& o, const DebugPrint<T>& d) {
        if (d.m_alpha == 256) {
            o << "[";
            for (auto c: d.m_view) {
                o << char(uint8_t(c));
            }
            o << "]";
            return o;
        } else {
            return o << vec_to_debug_string(d.m_view, 2);
        }
    }
    template<typename T>
    DebugPrint<T> debug_p(ConstGenericView<T> v, size_t alpha) {
        return DebugPrint<T>(v, alpha);
    }

    struct TypedBlock {
        uint8_t len;
        uint8_t type;
    };
    bool operator==(const TypedBlock& a, const TypedBlock& b) {
        return a.len == b.len && a.type == b.type;
    }
    std::ostream& operator<<(std::ostream& o, const TypedBlock& b) {
        return o << "{ len: " << int(b.len) << ", type: " << int(b.type) << " }";
    }

    template<typename T, size_t N>
    class FixedVector {
        std::array<T, N> m_array;
        size_t m_size = 0;
    public:
        inline FixedVector() {}
        inline void push_back(T&& t) {
            DCHECK_LT(m_size, N);
            m_array[m_size] = std::move(t);
            m_size++;
        }
        inline T pop_back() {
            DCHECK_GT(m_size, 0);
            m_size--;
            return std::move(m_array[m_size]);
        }
        inline T pop_front() {
            DCHECK_GT(m_size, 0);
            auto e = std::move(m_array[0]);

            for(size_t i = 0; i < m_size - 1; i++) {
                m_array[i] = m_array[i + 1];
            }
            m_size--;

            return std::move(e);
        }
        inline bool full() {
            return m_size == N;
        }
        inline GenericView<T> view() {
            return GenericView<T> { m_array.data(), m_size };
        }
    };

    std::ostream& nice_block_lengths(GenericView<TypedBlock> tbs, std::ostream& o) {
        for (auto& tb: tbs) {
            if (tb.len == 1) {
                o << "[" << int(tb.type) << "]";
            } else if (tb.len == 2) {
                o << "[  " << int(tb.type) << " ]";
            } else if (tb.len == 3) {
                o << "[   " << int(tb.type) << "   ]";
            } else {
                o << "<Err: " << tb <<  ">";
            }
        }
        return o;
    }

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

    template<typename Source>
    struct Context {
        size_t alphabet_size;
        std::vector<size_t> scratchpad;
        Source s;
        Source sb;
        size_t i = 0;
        size_t last_i = 0;
        bool print_mb_trace = true;
        bool print_mb2_trace = true;
        bool print_only_adjusted = false;
        bool print_bulk_text = false;

        std::vector<TypedBlock> block_buffer;

        Context(size_t as, Source src):
            alphabet_size(as),
            scratchpad(),
            s(src),
            sb(src),
            i(0),
            last_i(0)
        {}

        void reset_debug_print() {
            sb = s;
        }

        void check_sizes(string_ref errmsg) {
            size_t full_size = 0;
            for (auto& b : block_buffer) {
                full_size += b.len;
            }
            DCHECK_EQ(full_size, s.size()) << errmsg;
        }

        void print_cut(size_t l, size_t type, bool doit) {
            auto front_cut = sb.substr(0, l);
            auto back_cut = sb.substr(l);

            auto n = s.size() - (back_cut.size() + front_cut.size());
            n *= debug_p(sb, alphabet_size).char_mult();

            IF_DEBUG(if (doit && print_mb_trace) {
                std::cout << "mblock " << type << ": ";
                if (print_bulk_text) std::cout << std::setw(n) << "";
                std::cout << debug_p(front_cut, alphabet_size);
                if (print_bulk_text) std::cout << " ";
                if (print_bulk_text) std::cout << debug_p(back_cut, alphabet_size);
                std::cout << "\n";
                if (l < 2 || l > 3) std::cout << "Needs adjustment!\n";
            })

            sb = back_cut;
        }

        void print_all(bool doit) {
            reset_debug_print();
            for (auto& b : block_buffer) {
                print_cut(b.len, b.type, doit);
            }
        }

        void push_back(size_t l, size_t type) {
            print_cut(l, type, !print_only_adjusted);
            i += l;
            block_buffer.push_back(TypedBlock { uint8_t(l), uint8_t(type) });
        }

        void check_advanced(size_t len) {
            DCHECK_EQ(i - last_i, len);
            last_i = i;
        }

        std::vector<TypedBlock>& adjusted_blocks() {
            check_sizes("pre adjust");
            adjust_blocks(block_buffer);
            check_sizes("post adjust");

            print_all(true);
            for (auto& b: block_buffer) {
                DCHECK_GE(b.len, 2);
                DCHECK_LE(b.len, 3);
            }
            return block_buffer;
        }
    };

    template<typename LmPred, typename SpanPush>
    void landmark_spanner(size_t size,
                          LmPred pred,
                          SpanPush push,
                          bool tie) {
        struct Block {
            size_t left;
            size_t right;
            inline size_t size() { return right - left + 1; }
        };
        std::array<Block, 3> blocks {{
            {0, 0},
            {0, 0},
        }};
        uint8_t bi = 0;

        for(size_t i = 0; i < size; i++) {
            if (pred(i)) {
                blocks[1].left  = (i == 0)        ? (i) : (i - 1);
                blocks[1].right = (i == size - 1) ? (i) : (i + 1);
                /*std::cout <<
                    "i: " << i << ", "
                    "old: (" << left << ", " << right << "), "
                    "new: (" << new_left << ", " << new_right << ")\n";*/

                if (bi > 0) {
                    // Adjust overlap of adjacent landmarks
                    if (blocks[1].left == blocks[0].right) {
                        if (tie) {
                            blocks[0].right--;
                        } else {
                            blocks[1].left++;
                        }
                    }
                }

                if (bi == 0) {
                    bi = 1;
                } else {
                    push(blocks[0].left, blocks[0].right);
                }
                for (size_t j = 0; j < 1; j++) {
                    blocks[j] = blocks[j + 1];
                }
            }
        }
        if (bi == 1) {
            // entered at least once, one unused block
            push(blocks[1].left, blocks[1].right);
        }
    }
}
}
