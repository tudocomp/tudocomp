#include "tudocomp/tudocomp.hpp"
#include "tudocomp/compressors/EspCompressor.hpp"
#include "test/util.hpp"
#include <gtest/gtest.h>

#include <tudocomp/compressors/esp/meta_blocks.hpp>
#include <tudocomp/compressors/esp/tree_reducer.hpp>

using namespace tdc;

TEST(ESP, test) {
    std::vector<uint8_t> small_alpha = "abcabageheadbag"_v;
    //for (auto& e : small_alpha) { e -= 'a'; }

    // TODO: factor out recution code to be paramtric over
    // alphabet size

    std::vector<View> cases {
        "0000dkasxxxcsdacjzsbkhvfaghskcbsaaaaaaaaaaaaaaaaaadkcbgasdbkjcbackscfa",
        "bananaoobananaoobananaoobananaoo",
        "aaaaa",
        "asdf",
        "aaaxaaa",
        //"",
        "a",
        "aa",
        "aaa",
        "aaaa",
        "aaaaa",
        "aaaaaa",
        "a",
        "as",
        "asd",
        "asdf",
        "asdfg",
        "asdfgh",
        small_alpha,
    };

    for (auto& c : cases) {
        Input i(c);
        std::vector<uint8_t> v;
        Output o(v);

        auto comp = tdc::create_algo<EspCompressor>();

        comp.compress(i, o);

        std::cout << "\n";
    }
}


struct Empty {};

std::ostream& operator<<(std::ostream& os, Empty e) {
    return os << "Empty";
}

template<class Prev, class T, class F>
struct Context;

template<class Self, class T>
struct ContextBase {
    std::deque<T> buffer;
    size_t offset = -1;
    size_t min_index = -1;
    size_t debug_depth = 0;

    template<class G>
    auto stage(G g) -> Context<Self, decltype(g((Self&) *this, 0)), G> {
        auto r = Context<Self, decltype(g((Self&) *this, 0)), G> {
            (Self&) *this,
            g,
        };

        r.debug_depth = debug_depth + 1;

        return r;
    }

    void print_debug() {
        std::cout << "@" << debug_depth << " buffer size: " << buffer.size() << "\n";
    }
};

template<class Prev, class T, class F>
struct Context: ContextBase<Context<Prev, T, F>, T> {
    Prev prev;
    F f;

    Context(Prev prev_, F f_): prev(prev_), f(f_) {}

    T unbuffered_at(size_t i) {
        auto& cx = prev;
        cx.min_index = -1;
        auto r = f(cx, i);

        /*
        auto debug_old_size = cx.buffer.size();
        auto debug_old_offset = cx.offset;
        auto debug_old_min_index = cx.min_index;
        */

        if (cx.min_index != size_t(-1)) {
            for (size_t i = 0; i < (cx.min_index - cx.offset); i++) {
                cx.buffer.pop_front();
            }
            cx.offset += (cx.min_index - cx.offset);
        }

        /*
        std::cout
            << std::string(this->debug_depth * 4, ' ')
            << "[@" << i << ": "
            << "len: (" << debug_old_size
                << " -> " << cx.buffer.size()
            << "), offset: (" << (debug_old_offset == size_t(-1) ? -1 : int(debug_old_offset))
                << " -> " << (cx.offset == size_t(-1) ? -1 : int(cx.offset))
            << "), min_index: (" << (debug_old_min_index == size_t(-1) ? -1 : int(debug_old_min_index))
                << " -> " << (cx.min_index == size_t(-1) ? -1 : int(cx.min_index))
            << ")]\n";
        */

        return r;
    }

    T operator[](size_t i) {
        this->min_index = std::min(this->min_index, i);
        if (this->offset == size_t(-1)) {
            this->offset = i;
        }
        ptrdiff_t lokal_i = ptrdiff_t (i) - ptrdiff_t (this->offset);

        if (lokal_i < 0) {
            throw std::runtime_error("to bad");
        }

        while (size_t(lokal_i) >= this->buffer.size()) {
            this->buffer.push_back(unbuffered_at(this->buffer.size() + this->offset));
        }

        return this->buffer[lokal_i];
    }

    void print_debug() {
        prev.print_debug();
        ContextBase<Context<Prev, T, F>, T>::print_debug();
    }
};

struct InitialContext: ContextBase<InitialContext, Empty> {
    InitialContext() {}
};

template<class Prev, class T, class F>
struct VectorElementContext: ContextBase<VectorElementContext<Prev, T, F>, T> {
    std::shared_ptr<F> f;
    std::shared_ptr<Prev> first_prev;
    std::shared_ptr<VectorElementContext<Prev, T, F>> other_prev;
    size_t vector_index;
};

template<class Prev, class T, class F>
struct VectorContext: ContextBase<VectorContext<Prev, T, F>, T> {
    std::shared_ptr<Prev> prev;
    std::shared_ptr<F> f;
    std::vector<VectorElementContext<Prev, T, F>> elements;
};

TEST(OffsetOp, test) {
    std::vector<uint8_t> v { 1, 5, 7, 3, 5, 2, 9, 0 };

    //////////////////////////////////////////

    using T0 = InitialContext;
    auto s0 = [&](T0& c, size_t i) -> uint32_t {
        return v.at(i);
    };

    //////////////////////////////////////////

    using T1 = Context<T0, uint32_t, decltype(s0)>;
    auto s1 = [&](T1& c, size_t j) -> uint32_t {
        auto x = c[j - 1];
        auto y = c[j];
        auto z = c[j + 1];

        std::cout << int(x) << ", " << int(y) << ", " << int(z) << "\n";

        return x * 100 + y * 10 + z;
    };

    //////////////////////////////////////////

    using T2 = Context<T1, uint32_t, decltype(s1)>;
    auto s2 = [&](T2& c, size_t k) -> uint32_t {
        auto a = c[k];
        auto b = c[k + 1];

        std::cout << int(a) << ", " << int(b) << "\n";

        return a * 1000 + b;
    };

    //////////////////////////////////////////

    auto chain = InitialContext().stage(s0).stage(s1).stage(s2);

    //////////////////////////////////////////

    for (size_t k = 1; k < v.size() - 2; k++) {
        std::cout << chain.unbuffered_at(k) << "\n";
    }

    //////////////////////////////////////////

    std::cout << "\n";
    chain.print_debug();

}

void landmark_spanner_test(std::vector<int> landmarks,
                           std::vector<std::array<size_t, 2>> should_spans,
                           bool tie) {
    std::vector<std::array<size_t, 2>> spans;
    esp::landmark_spanner(landmarks.size(),
                          [&](size_t i) { return landmarks[i] == 1; },
                          [&](size_t i, size_t j) {
                                spans.push_back({i, j});
                          },
                          tie);

    ASSERT_EQ(should_spans, spans) << "Unadjusted spans don't match";
}

void landmark_spanner_test_adj(std::vector<int> landmarks,
                               std::vector<std::array<size_t, 2>> should_spans,
                               bool tie) {
    std::vector<std::array<size_t, 2>> spans;
    esp::landmark_spanner(landmarks.size(),
                          [&](size_t i) { return landmarks[i] == 1; },
                          [&](size_t i, size_t j) {
                                spans.push_back({i, j});
                          },
                          tie);

    std::vector<esp::TypedBlock> v;
    for(auto span : spans) {
        v.push_back(esp::TypedBlock {
            uint8_t(span[1] - span[0] + 1), 2
        });
    }

    esp::adjust_blocks(v);

    spans.clear();
    size_t i = 0;
    for (auto b : v) {
        spans.push_back({ i, i + b.len - 1});
        i += b.len;
    }

    ASSERT_EQ(should_spans, spans) << "Adjusted spans don't match";
}

TEST(Esp, landmark_spanner_1) {
    landmark_spanner_test(
        { 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1 },
        {
            {0, 0},
            {1, 2},
            {3, 5},
            {6, 7},
            {8, 9},
            {10, 11},
        },
        true
    );
    landmark_spanner_test_adj(
        { 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1 },
        {
            {0, 2},
            {3, 5},
            {6, 7},
            {8, 9},
            {10, 11},
        },
        true
    );
}

TEST(Esp, landmark_spanner_2) {
    landmark_spanner_test(
        { 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1 },
        {
            {0, 1},
            {2, 3},
            {4, 5},
            {6, 8},
            {9, 10},
            {11, 11},
        },
        false
    );
    landmark_spanner_test_adj(
        { 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1 },
        {
            {0, 1},
            {2, 3},
            {4, 5},
            {6, 8},
            {9, 11},
        },
        false
    );
}

TEST(Esp, landmark_spanner_3) {
    landmark_spanner_test({ 1, 0 }, {{0, 1}}, false);
    landmark_spanner_test({ 1, 0 }, {{0, 1}}, true);
    landmark_spanner_test_adj({ 1, 0 }, {{0, 1}}, false);
    landmark_spanner_test_adj({ 1, 0 }, {{0, 1}}, true);
}
TEST(Esp, landmark_spanner_4) {
    landmark_spanner_test({ 0, 1 }, {{0, 1}}, false);
    landmark_spanner_test({ 0, 1 }, {{0, 1}}, true);
    landmark_spanner_test_adj({ 0, 1 }, {{0, 1}}, false);
    landmark_spanner_test_adj({ 0, 1 }, {{0, 1}}, true);
}
TEST(Esp, landmark_spanner_5) {
    landmark_spanner_test({ 0, 1, 0 }, {{0, 2}}, false);
    landmark_spanner_test({ 0, 1, 0 }, {{0, 2}}, true);
    landmark_spanner_test_adj({ 0, 1, 0 }, {{0, 2}}, false);
    landmark_spanner_test_adj({ 0, 1, 0 }, {{0, 2}}, true);
}
TEST(Esp, landmark_spanner_6) {
    landmark_spanner_test({ 1, 0, 1 }, {{0, 1}, {2, 2}}, false);
    landmark_spanner_test({ 1, 0, 1 }, {{0, 0}, {1, 2}}, true);
    landmark_spanner_test_adj({ 1, 0, 1 }, {{0, 2}}, false);
    landmark_spanner_test_adj({ 1, 0, 1 }, {{0, 2}}, true);
}
TEST(Esp, landmark_spanner_7) {
    landmark_spanner_test({ 1, 0, 1, 0 }, {{0, 1}, {2, 3}}, false);
    landmark_spanner_test({ 1, 0, 1, 0 }, {{0, 0}, {1, 3}}, true);
    landmark_spanner_test_adj({ 1, 0, 1, 0 }, {{0, 1}, {2, 3}}, false);
    landmark_spanner_test_adj({ 1, 0, 1, 0 }, {{0, 1}, {2, 3}}, true);
}
TEST(Esp, landmark_spanner_8) {
    landmark_spanner_test({ 1, 0, 0, 1 }, {{0, 1}, {2, 3}}, false);
    landmark_spanner_test({ 1, 0, 0, 1 }, {{0, 1}, {2, 3}}, true);
    landmark_spanner_test_adj({ 1, 0, 0, 1 }, {{0, 1}, {2, 3}}, false);
    landmark_spanner_test_adj({ 1, 0, 0, 1 }, {{0, 1}, {2, 3}}, true);
}
TEST(Esp, landmark_spanner_9) {
    landmark_spanner_test({ 0, 1, 0, 1 }, {{0, 2}, {3, 3}}, false);
    landmark_spanner_test({ 0, 1, 0, 1 }, {{0, 1}, {2, 3}}, true);
    landmark_spanner_test_adj({ 0, 1, 0, 1 }, {{0, 1}, {2, 3}}, false);
    landmark_spanner_test_adj({ 0, 1, 0, 1 }, {{0, 1}, {2, 3}}, true);
}
TEST(Esp, landmark_spanner_10) {
    landmark_spanner_test({ 0, 1, 0, 1, 0 }, {{0, 2}, {3, 4}}, false);
    landmark_spanner_test({ 0, 1, 0, 1, 0 }, {{0, 1}, {2, 4}}, true);
    landmark_spanner_test_adj({ 0, 1, 0, 1, 0 }, {{0, 2}, {3, 4}}, false);
    landmark_spanner_test_adj({ 0, 1, 0, 1, 0 }, {{0, 1}, {2, 4}}, true);
}
TEST(Esp, landmark_spanner_11) {
    landmark_spanner_test({ 0, 1, 0, 0, 1 }, {{0, 2}, {3, 4}}, false);
    landmark_spanner_test({ 0, 1, 0, 0, 1 }, {{0, 2}, {3, 4}}, true);
    landmark_spanner_test_adj({ 0, 1, 0, 0, 1 }, {{0, 2}, {3, 4}}, false);
    landmark_spanner_test_adj({ 0, 1, 0, 0, 1 }, {{0, 2}, {3, 4}}, true);
}
TEST(Esp, landmark_spanner_12) {
    landmark_spanner_test({ 1, 0, 0, 1, 0 }, {{0, 1}, {2, 4}}, false);
    landmark_spanner_test({ 1, 0, 0, 1, 0 }, {{0, 1}, {2, 4}}, true);
    landmark_spanner_test_adj({ 1, 0, 0, 1, 0 }, {{0, 1}, {2, 4}}, false);
    landmark_spanner_test_adj({ 1, 0, 0, 1, 0 }, {{0, 1}, {2, 4}}, true);
}
TEST(Esp, landmark_spanner_13) {
    landmark_spanner_test({ 1, 0, 1, 0, 1 }, {{0, 1}, {2, 3}, {4, 4}}, false);
    landmark_spanner_test({ 1, 0, 1, 0, 1 }, {{0, 0}, {1, 2}, {3, 4}}, true);
    landmark_spanner_test_adj({ 1, 0, 1, 0, 1 }, {{0, 1}, {2, 4}}, false);
    landmark_spanner_test_adj({ 1, 0, 1, 0, 1 }, {{0, 2}, {3, 4}}, true);
}


void split_test(string_ref s) {
    esp::Context<decltype(s)> ctx {
        256,
        s,
    };
    ctx.print_mb2_trace = false;

    std::cout << "             [" << s << "]\n";
    esp::split(s, ctx);
    std::cout << "\n[Adjusted]:\n\n";
    ctx.adjusted_blocks();
}

TEST(Esp, new_split) {
    split_test(
        "0000dkasxxxcsdacjzsbkhvfaghskcbsaaaaaaaaaaaaaaaaaadkcbgasdbkjcbackscfa"_v
    );
}

TEST(Esp, new_split_2) {
    split_test("adkcbbackscfaaaaafaaaa"_v);
}

TEST(Esp, new_split_3) {
    split_test("adkcbbackscfaaaaafaaaa"_v);
}

TEST(Esp, new_split_4) {
    split_test("faaaa"_v);
}

TEST(Esp, new_split_5) {
    split_test("aaaaf"_v);
}

TEST(Esp, new_split_6) {
    split_test("faa"_v);
}

TEST(Esp, new_split_7) {
    split_test("aaf"_v);
}

TEST(Esp, new_split_8) {
    split_test("faaa"_v);
}

TEST(Esp, new_split_9) {
    split_test("aaaf"_v);
}

TEST(Esp, new_split_10) {
    split_test("faaaaa"_v);
}

TEST(Esp, new_split_11) {
    split_test("aaaaaf"_v);
}

TEST(Esp, new_split_12) {
    split_test("faaaaaa"_v);
}

TEST(Esp, new_split_13) {
    split_test("aaaaaaf"_v);
}

void test_adjust_blocks(std::vector<esp::TypedBlock> a,
                        std::vector<esp::TypedBlock> b) {
    adjust_blocks(a);
    ASSERT_EQ(a, b);
}


TEST(Esp, adjust_block_1) {
    test_adjust_blocks(
        { esp::TypedBlock { 2, 3 }, esp::TypedBlock { 3, 2 } },
        { esp::TypedBlock { 2, 3 }, esp::TypedBlock { 3, 2 } }
    );
}

TEST(Esp, adjust_block_2) {
    test_adjust_blocks(
        { esp::TypedBlock { 2, 3 }, esp::TypedBlock { 1, 2 }, esp::TypedBlock { 2, 2 } },
        { esp::TypedBlock { 2, 3 }, esp::TypedBlock { 3, 2 } }
    );
}

TEST(Esp, tree_reducer) {
    auto r = esp::generate_grammar_rounds(
        "0000dkasxxxcsdacjzsbkhvfaghskcbs"
        "aaaaaaaaaaaaaaaaaadkcbgasdbkjcbackscfa"
    );

    auto s = esp::generate_grammar(r);

    for (size_t i = 0; i < s.size(); i++) {
        std::cout << i << " -> " << vec_to_debug_string(s[i]) << "\n";
    }
}
