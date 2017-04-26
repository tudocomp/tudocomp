#include "test/util.hpp"
#include <gtest/gtest.h>

#include "tudocomp/compressors/EspCompressor.hpp"

#include "tudocomp/compressors/esp/SLPDepSort.hpp"

#include "tudocomp/compressors/esp/PlainSLPStrategy.hpp"
#include "tudocomp/compressors/esp/SufficientSLPStrategy.hpp"
#include "tudocomp/compressors/esp/MonotoneSubsequences.hpp"

#include "tudocomp/compressors/EspCompressor.hpp"

using namespace tdc;

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
    esp::RoundContext<decltype(s)> ctx {
        256,
        s,
        true, // max repeating meta blocks
        true, // tie to right (or left?)
        esp::DebugRoundContext(std::cout, true, true),
    };

    std::cout << "             [" << s << "]\n";
    ctx.split(s);
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

TEST(Esp, tree_reducer_roundtrip) {
    auto s = "0000dkasxxxcsdacjzsbkhvfaghskcbs"
             "aaaaaaaaaaaaaaaaaadkcbgasdbkjcbackscfa"_v;

    esp::EspContext esp {
        nullptr, // no env
        false,   // not silent
    };

    std::cout << "\n[Complete Grammar]:\n\n";
    auto slp = esp.generate_grammar(s);
    for (size_t i = 0; i < slp.rules.size(); i++) {
        std::cout
            << i << ": "
            << i + esp::GRAMMAR_PD_ELLIDED_PREFIX
            << " -> (" << slp.rules[i][0] << ", " << slp.rules[i][1] << ")\n";
    }

    std::cout << "start rule: " << slp.root_rule << "\n";

    auto s2 = slp.derive_text_s();

    std::cout << "\n[Derived String]:\n\n";
    std::cout << s2 << "\n";

    ASSERT_EQ(s, s2);

}

template<typename T>
void test_esp() {
 // TODO: ensure ESP code is parametric over input alphabet size and format

    std::vector<string_ref> cases {
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
        "abcabageheadbag",
        "",
        "報チ申猛あち涙境ワセ周兵いわ郵入せすをだ漏告されて話巡わッきや間紙あいきり諤止テヘエラ鳥提フ健2銀稿97傷エ映田ヒマ役請多暫械ゅにうて。関国ヘフヲオ場三をおか小都供セクヲ前俳著ゅ向深まも月10言スひす胆集ヌヱナ賀提63劇とやぽ生牟56詰ひめつそ総愛ス院攻せいまて報当アラノ日府ラのがし。",
    };

    for (auto& c : cases) {
        std::cout << "---------------------\n";
        test::roundtrip<EspCompressor<T>>(c);
    }
}

TEST(ESP, test_plain) {
   test_esp<esp::PlainSLPStrategy>();
}

TEST(ESP, test_sufficient) {
   test_esp<esp::SufficientSLPStrategy>();
}

class IBST {
    size_t m_node;
    std::vector<IBST> m_children;
public:
    IBST(size_t value): m_node(value) {}
    void add_child(IBST&& ibst) {
        m_children.push_back(std::move(ibst));
    }
};

void inverse_deps(const esp::SLP& slp, size_t root_node, std::vector<size_t>& inv, size_t ind = 0) {
    if (inv[root_node] != size_t(-1)) {
        return;
    } else if (root_node <= 255) {
        inv[root_node] = 0;
        return;
    }

    auto& x = slp.node(root_node);
    auto a = x[0];
    auto b = x[1];


    //std::cout << std::setw(ind) << "" << a << " -> " << root_node << "\n";
    inverse_deps(slp, a, inv, ind + 2);
    //std::cout << std::setw(ind) << "" << b << " -> " << root_node << "\n";
    inverse_deps(slp, b, inv, ind + 2);
}

TEST(DepSort, test) {
    esp::SLP slp;

    slp.rules = std::vector<std::array<size_t, 2>> {
        {48, 48},
        {100, 107},
        {97, 115},
        {298, 120},
        {300, 100},
        {97, 99},
        {106, 122},
        {299, 107},
        {303, 102},
        {297, 104},
        {115, 107},
        {302, 115},
        {296, 97},
        {295, 99},
        {98, 103},
        {100, 98},
        {301, 99},
        {98, 97},
        {99, 107},
        {115, 99},
        {102, 97},
        {256, 256},
        {305, 259},
        {260, 261},
        {262, 263},
        {264, 265},
        {266, 267},
        {306, 268},
        {307, 258},
        {308, 273},
        {304, 276},
        {277, 278},
        {279, 280},
        {281, 282},
        {283, 283},
        {309, 286},
        {310, 289},
        {290, 291},
        {292, 293},
        {100, 107},
        {97, 97},
        {97, 103},
        {120, 120},
        {115, 98},
        {99, 115},
        {107, 106},
        {99, 98},
        {104, 118},
        {274, 275},
        {257, 258},
        {268, 268},
        {269, 270},
        {271, 272},
        {284, 285},
        {287, 288},
    };
    slp.root_rule = 294;
    slp.empty = false;

    auto slp_test = [&]() {
        std::cout << "[SLP Grammar]:\n";
        for (size_t i = 0; i < slp.rules.size(); i++) {
            auto x = i + esp::GRAMMAR_PD_ELLIDED_PREFIX;
            std::cout
                << "  "
                << x
                << " -> ("
                << slp.rules[i][0]
                << ", "
                << slp.rules[i][1]
                << ")\n";
        }
        auto s = slp.derive_text_s();

        ASSERT_EQ(s, "0000dkasxxxcsdacjzsbkhvfaghskcbsaaaaaaaaaaaaaaaaaadkcbgasdbkjcbackscfa"_v);
        std::cout
            << "Derived text:\n"
            << s
            << "\n";
    };

    slp_test();

    esp::slp_dep_sort(slp);

    slp_test();
}

const std::vector<size_t> SUBSEQ_TEST_INPUT { 2, 3, 2, 8, 0, 5, 1, 3, 6, 6, 0, 4, 4, 1, 6 };

TEST(MonotonSubseq, init_afwd) {
    auto sis = esp::sorted_indices(SUBSEQ_TEST_INPUT);
    std::vector<esp::Point> points;
    for (size_t i = 0; i < sis.size(); i++) {
        points.push_back(esp::point_coord_if_at_index(sis[i], i));
    }

    ASSERT_EQ(sis, (std::vector<size_t> {
        4, 10, 6, 13, 0, 2, 1, 7, 11, 12, 5, 8, 9, 14, 3
    }));

    ASSERT_EQ(points, (std::vector<esp::Point> {
        {0, 4}, {1, 10}, {2, 6}, {3, 13}, {4, 0}, {5, 2}, {6, 1}, {7, 7},
        {8, 11}, {9, 12}, {10, 5}, {11, 8}, {12, 9}, {13, 14}, {14, 3},
    }));
}

TEST(MonotonSubseq, init_bbwd) {
    auto sis = esp::sorted_indices(SUBSEQ_TEST_INPUT);
    std::reverse(sis.begin(), sis.end());
    std::vector<esp::Point> points;
    for (size_t i = 0; i < sis.size(); i++) {
        points.push_back(esp::point_coord_if_at_index(sis[i], i));
    }

    ASSERT_EQ(sis, (std::vector<size_t> {
        3, 14, 9, 8, 5, 12, 11, 7, 1, 2, 0, 13, 6, 10, 4,
    }));

    ASSERT_EQ(points, (std::vector<esp::Point> {
        {0, 3}, {1, 14}, {2, 9}, {3, 8}, {4, 5}, {5, 12}, {6, 11}, {7, 7},
        {8, 1}, {9, 2}, {10, 0}, {11, 13}, {12, 6}, {13, 10}, {14, 4},
    }));
}

TEST(MonotonSubseq, layers_iter_afwd) {
    auto rev = false;
    auto sis = esp::sorted_indices(SUBSEQ_TEST_INPUT);
    auto l = esp::L(sis);

    auto layers_iter = esp::LayersIterator(l, rev);
    std::vector<esp::Point> points;
    while (layers_iter.has_next()) {
        points.push_back(esp::point_coord_for_link(sis, layers_iter.advance(), rev));
    }
    std::reverse(points.begin(), points.end());

    ASSERT_EQ(points, (std::vector<esp::Point> {
        {0, 4}, {1, 10}, {2, 6}, {3, 13}, {4, 0}, {5, 2}, {6, 1}, {7, 7},
        {8, 11}, {9, 12}, {10, 5}, {11, 8}, {12, 9}, {13, 14}, {14, 3},
    }));
}

TEST(MonotonSubseq, layers_iter_bbwd) {
    auto rev = true;
    auto sis = esp::sorted_indices(SUBSEQ_TEST_INPUT);
    auto l = esp::L(sis);

    auto layers_iter = esp::LayersIterator(l, rev);
    std::vector<esp::Point> points;
    while (layers_iter.has_next()) {
        points.push_back(esp::point_coord_for_link(sis, layers_iter.advance(), rev));
    }
    std::reverse(points.begin(), points.end());

    ASSERT_EQ(points, (std::vector<esp::Point> {
        {0, 3}, {1, 14}, {2, 9}, {3, 8}, {4, 5}, {5, 12}, {6, 11}, {7, 7},
        {8, 1}, {9, 2}, {10, 0}, {11, 13}, {12, 6}, {13, 10}, {14, 4},
    }));
}

TEST(MonotonSubseq, build_layers_afwd) {
    auto rev = false;
    auto sis = esp::sorted_indices(SUBSEQ_TEST_INPUT);

    auto l = esp::L(sis);
    l.rebuild(rev);

    ASSERT_EQ(l.layers_size(), 6);

    auto dbg = l.to_debug_layer_points();

    ASSERT_EQ(dbg, (std::vector<std::vector<esp::Point>> {
        {{13, 14}, {14, 3}},
        {{3, 13}, {9, 12}, {12, 9}},
        {{8, 11}, {11, 8}},
        {{1, 10}, {7, 7}, {10, 5}},
        {{2, 6}, {5, 2}, {6, 1}},
        {{0, 4}, {4, 0}},
    }));
}

TEST(MonotonSubseq, build_layers_bbwd) {
    auto rev = true;
    auto sis = esp::sorted_indices(SUBSEQ_TEST_INPUT);

    auto l = esp::L(sis);
    l.rebuild(rev);

    ASSERT_EQ(l.layers_size(), 4);

    auto dbg = l.to_debug_layer_points();

    ASSERT_EQ(dbg, (std::vector<std::vector<esp::Point>> {
        {{1, 14}, {11, 13}, {13, 10}, {14, 4}},
        {{5, 12}, {6, 11}, {7, 7}, {12, 6}},
        {{2, 9}, {3, 8}, {4, 5}, {9, 2}, {10, 0}},
        {{0, 3}, {8, 1}},
    }));
}

TEST(MonotonSubseq, build_extract_layers_afwd) {
    auto rev = false;
    auto sis = esp::sorted_indices(SUBSEQ_TEST_INPUT);

    auto l = esp::L(sis);
    l.rebuild(rev);

    std::vector<size_t> links;
    l.lis(l.layers_size(), links);

    std::vector<size_t> indices;
    for (auto link : links) {
        indices.push_back(l.sindex_for_link(link));
    }

    std::vector<size_t> values;
    for (auto index: indices) {
        values.push_back(SUBSEQ_TEST_INPUT[index]);
    }

    ASSERT_EQ(indices, (std::vector<size_t> { 4, 6, 7, 11, 12, 14 }));
    ASSERT_EQ(values,  (std::vector<size_t> { 0, 1, 3, 4, 4, 6 }));
}

TEST(MonotonSubseq, build_extract_layers_bbwd) {
    auto rev = true;
    auto sis = esp::sorted_indices(SUBSEQ_TEST_INPUT);

    auto l = esp::L(sis);
    l.rebuild(rev);

    std::vector<size_t> links;
    l.lis(l.layers_size(), links);

    std::vector<size_t> indices;
    for (auto link : links) {
        indices.push_back(l.sindex_for_link(link));
    }

    std::vector<size_t> values;
    for (auto index: indices) {
        values.push_back(SUBSEQ_TEST_INPUT[index]);
    }

    ASSERT_EQ(indices, (std::vector<size_t> { 3, 9, 12, 13 }));
    ASSERT_EQ(values,  (std::vector<size_t> { 8, 6, 4, 1 }));
}

TEST(MonotonSubseq, lis_corner_cases) {
    auto rev = false;
    auto sis = esp::sorted_indices(SUBSEQ_TEST_INPUT);

    auto l = esp::L(sis);
    l.rebuild(rev);

    std::vector<size_t> links;
    ASSERT_TRUE(l.lis(l.layers_size(), links));
    ASSERT_FALSE(l.lis(l.layers_size() + 1, links));
    ASSERT_TRUE(l.lis(l.layers_size() - 1, links));
    ASSERT_TRUE(l.lis(1, links));
    ASSERT_TRUE(l.lis(0, links));
}

TEST(MonotonSubseq, build_extract_remove_layers_afwd) {
    auto rev = false;
    auto sis = esp::sorted_indices(SUBSEQ_TEST_INPUT);

    auto l = esp::L(sis);
    l.rebuild(rev);

    std::vector<size_t> links;
    l.lis(l.layers_size(), links);

    std::vector<size_t> indices;
    for (auto link : links) {
        indices.push_back(l.sindex_for_link(link));
    }

    std::vector<size_t> values;
    for (auto index: indices) {
        values.push_back(SUBSEQ_TEST_INPUT[index]);
    }

    l.remove_all_and_slice(links);

    ASSERT_EQ(indices, (std::vector<size_t> { 4, 6, 7, 11, 12, 14 }));
    ASSERT_EQ(values,  (std::vector<size_t> { 0, 1, 3, 4, 4, 6 }));
    ASSERT_EQ(sis, (std::vector<size_t> { 4, 10, 6, 13, 0, 2, 1, 7, 11, 12, 5, 8, 9, 14, 3 }));
}

TEST(MonotonSubseq, build_extract_remove_layers_bbwd) {
    auto rev = true;
    auto sis = esp::sorted_indices(SUBSEQ_TEST_INPUT);

    auto l = esp::L(sis);
    l.rebuild(rev);

    std::vector<size_t> links;
    l.lis(l.layers_size(), links);

    std::vector<size_t> indices;
    for (auto link : links) {
        indices.push_back(l.sindex_for_link(link));
    }

    std::vector<size_t> values;
    for (auto index: indices) {
        values.push_back(SUBSEQ_TEST_INPUT[index]);
    }

    l.remove_all_and_slice(links);

    ASSERT_EQ(indices, (std::vector<size_t> { 3, 9, 12, 13 }));
    ASSERT_EQ(values,  (std::vector<size_t> { 8, 6, 4, 1 }));
    ASSERT_EQ(sis, (std::vector<size_t> { 4, 10, 6, 13, 0, 2, 1, 7, 11, 12, 5, 8, 9, 14, 3 }));
}

TEST(MonotonSubseq, iterative_longer_layers_round_to_inc) {
    auto sis = esp::sorted_indices(SUBSEQ_TEST_INPUT);
    std::vector<size_t> sis_n;

    {
        auto l = esp::L(sis);
        std::vector<esp::Link> links;
        while (l.sindices_size() > 0) {
            l.rebuild(false);
            l.lis(l.layers_size(), links);

            l.rebuild(true);
            // Only run lis() if needed:
            if (!(links.size() >= l.layers_size())) {
                l.lis(l.layers_size(), links);
            }

            // links now contains the longer sequence
            l.remove_all_and_slice(links);
        }
        sis_n = std::move(l).extract();
    }

    ASSERT_EQ(sis,   (std::vector<size_t> { 4, 10, 6, 13, 0, 2, 1, 7, 11, 12, 5, 8, 9, 14, 3 }));
    ASSERT_EQ(sis_n, (std::vector<size_t> { 0,  2, 0,  2, 1, 1, 3, 0, 0,   0, 1, 1, 1,  0, 3 }));
}

TEST(MonotonSubseq, iterative_longer_layers_round_to_dec) {
    auto sis = esp::sorted_indices(SUBSEQ_TEST_INPUT);
    std::vector<size_t> sis_n;

    {
        auto l = esp::L(sis);
        std::vector<esp::Link> links;
        while (l.sindices_size() > 0) {
            l.rebuild(false);
            l.lis(l.layers_size(), links);

            l.rebuild(true);
            // Only run lis() if needed:
            if (!(links.size() > l.layers_size())) {
                l.lis(l.layers_size(), links);
            }

            // links now contains the longer sequence
            l.remove_all_and_slice(links);
        }
        sis_n = std::move(l).extract();
    }

    ASSERT_EQ(sis,   (std::vector<size_t> { 4, 10, 6, 13, 0, 2, 1, 7, 11, 12, 5, 8, 9, 14, 3 }));
    ASSERT_EQ(sis_n, (std::vector<size_t> { 0,  3, 0,  2, 1, 1, 3, 0, 0,   0, 1, 1, 1,  0, 2 }));
}

TEST(MonotonSubseq, esp_encoding_paper_example) {
    auto inp = std::vector<size_t> { 0, 1, 1, 0, 4 };

    auto sis = std::vector<size_t> { 0, 3, 1, 2, 4 };
    auto Dpi = std::vector<size_t> { 0, 1, 1, 0, 0 };

    // create from Dpi
    auto Dsi = std::vector<size_t> {};
    {
        Dsi.reserve(inp.size());
        Dsi.resize(inp.size());
        for (size_t i = 0; i < sis.size(); i++) {
            Dsi[sis[i]] = Dpi[i];
        }
    }

    size_t last = 0;
    std::vector<size_t> values;
    std::vector<size_t> B;
    for (auto idx : sis) {
        values.push_back(inp[idx]);
        B.push_back(inp[idx] - last);
        last = inp[idx];
    }
    //std::cout << vec_to_debug_string(values) << "\n";
    //std::cout << vec_to_debug_string(B) << "\n";
    // calc b somehow

    // B as rank/select dictionary storen... or not because we just need compression
    //

    ASSERT_EQ(sis, (std::vector<size_t> { 0, 3, 1, 2, 4 }));
    ASSERT_EQ(Dsi, (std::vector<size_t> { 0, 1, 0, 1, 0 }));
    ASSERT_EQ(Dpi, (std::vector<size_t> { 0, 1, 1, 0, 0 }));
}

TEST(MonotonSubseq, esp_encoding_real1) {
    const auto D = std::vector<size_t>{
        48, 115, 99, 97, 103, 103, 97, 107, 98, 115, 107, 98, 107, 97, 118,
        122, 106, 107, 99, 98, 120, 256, 97, 104, 274, 115, 100, 257, 288, 99,
        102, 290, 99, 281, 107, 120, 296, 278, 269, 258, 291, 262, 261, 279,
        302, 278, 287, 257, 289, 304, 301, 297, 309, 310, 294
    };

    //std::cout << "D:\n" << vec_to_debug_string(D, 3) << "\n";

    auto sorted_indices = esp::sorted_indices(D);

    //std::cout << "sorted indices:\n" << vec_to_debug_string(sorted_indices) << "\n";

    std::vector<size_t> Bde;
    {
        for (auto index : sorted_indices) {
            Bde.push_back(D[index]);
        }
    }
    std::cout << "sorted D:\n" << vec_to_debug_string(Bde) << "\n";
    //std::cout << "emit unary coding B...\n";

    std::vector<size_t> Dpi;
    auto b = IntVector<uint_t<1>> {};
    {
        auto tmp = esp::create_dpi_and_b_from_sorted_indices(sorted_indices);
        Dpi = std::move(tmp.Dpi);
        b = std::move(tmp.b);
    }

    //std::cout << "Dpi:\n" << vec_to_debug_string(Dpi, 3) << "\n";

    // create from Dpi
    auto Dsi = esp::create_dsigma_from_dpi_and_sorted_indices(sorted_indices, Dpi);

    // std::cout << "Dsi:\n" << vec_to_debug_string(Dsi, 3) << "\n";

    /*
    {
        auto subseqs = std::vector<std::vector<size_t>>();
        for(size_t i = 0; i < D.size(); i++) {
            auto j = Dsi[i];
            while (j >= subseqs.size()) {
                subseqs.push_back({});
            }
            subseqs[j].push_back(D[i]);
        }

        std::cout << "Subseqs:\n";
        for (auto& e : subseqs) {
            std::cout << "    " << vec_to_debug_string(e) << "\n";
        }
    }*/

    // std::cout << "b:\n" << vec_to_debug_string(b) << "\n";

    // recover D
    std::vector<size_t> recovered_D;
    recovered_D.resize(Dpi.size());
    esp::recover_D_from_encoding(Dpi, Dsi, b, Bde, &recovered_D);

    //std::cout << "recovered_D:\n" << vec_to_debug_string(recovered_D, 3) << "\n";

    ASSERT_EQ(D, recovered_D);

    // wavelett trees
    {
        //std::cout << "Dsi:\n" << vec_to_debug_string(Dsi, 1) << "\n";
        auto Dsi_bvs = esp::make_wt(Dsi, b.size() - 1);
        auto recovered_Dsi = esp::recover_Dxx(Dsi_bvs, Dsi.size());
        //std::cout << vec_to_debug_string(recovered_Dsi) << "\n";
        ASSERT_EQ(Dsi, recovered_Dsi);

        //std::cout << "Dpi:\n" << vec_to_debug_string(Dpi, 1) << "\n";
        auto Dpi_bvs = esp::make_wt(Dpi, b.size() - 1);
        auto recovered_Dpi = esp::recover_Dxx(Dpi_bvs, Dsi.size());
        //std::cout << vec_to_debug_string(recovered_Dpi) << "\n";
        ASSERT_EQ(Dpi, recovered_Dpi);

    }
}



