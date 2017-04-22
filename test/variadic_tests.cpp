// tests for the various variadic compile-time utilities
// almost all are "implemented" using static assertions

#include <type_traits>
#include <tudocomp/util/cpp14/integer_sequence.hpp>
#include <tudocomp/util/integer_sequence_sort.hpp>
#include <tudocomp/util/type_list.hpp>

using namespace tdc;

// integer sequences
using mtseq_t    = std::index_sequence<>;
using testseq1_t = std::index_sequence<5, 2, 4, 3, 1>;

// sequence_element
static_assert(sequence_element<0, testseq1_t>::value == 5, "sequence_element");
static_assert(sequence_element<1, testseq1_t>::value == 2, "sequence_element");
static_assert(sequence_element<2, testseq1_t>::value == 4, "sequence_element");
static_assert(sequence_element<3, testseq1_t>::value == 3, "sequence_element");
static_assert(sequence_element<4, testseq1_t>::value == 1, "sequence_element");

// is::prepend
static_assert(std::is_same<
    is::prepend<size_t, 1, mtseq_t>,
    std::index_sequence<1>
>::value, "is::prepend on empty sequence");

static_assert(std::is_same<
    is::prepend<size_t, 1, std::index_sequence<2, 3>>,
    std::index_sequence<1, 2, 3>
>::value, "is::prepend on sequence");

// is::sort
static_assert(std::is_same<
    is::sort<size_t, mtseq_t>,
    mtseq_t
>::value, "is::sort on empty sequence");

static_assert(std::is_same<
    is::sort<size_t, testseq1_t, is::ascending>,
    std::index_sequence<1, 2, 3, 4, 5>
>::value, "is::sort ascending");

static_assert(std::is_same<
    is::sort<size_t, testseq1_t, is::descending>,
    std::index_sequence<5, 4, 3, 2, 1>
>::value, "is::sort descending");

// types and type lists
struct A;
struct B;
struct C;

using testtl1_t = tl::type_list<A, B, C>;

// tl::get
static_assert(std::is_same<tl::get<0, testtl1_t>, A>::value, "tl::get");
static_assert(std::is_same<tl::get<1, testtl1_t>, B>::value, "tl::get");
static_assert(std::is_same<tl::get<2, testtl1_t>, C>::value, "tl::get");

// tl::prepend
static_assert(std::is_same<
    tl::prepend<A, tl::mt>,
    tl::type_list<A>
>::value, "tl::prepend on empty type list");

static_assert(std::is_same<
    tl::prepend<A, tl::type_list<B, C>>,
    testtl1_t
>::value, "tl::prepend on type list");

// tl::set
static_assert(std::is_same<
    tl::set<0, A>,
    tl::type_list<A>
>::value, "tl::set case 1");

static_assert(std::is_same<
    tl::set<2, A>,
    tl::type_list<tl::None, tl::None, A>
>::value, "tl::set case 2");

// tl::mix
static_assert(std::is_same<
    tl::mix<tl::set<0, A>, tl::set<1, B>>,
    tl::type_list<A, B>
>::value, "tl::mix");

static_assert(std::is_same<
    tl::mix<testtl1_t, testtl1_t>,
    tl::type_list<tl::Ambiguous, tl::Ambiguous, tl::Ambiguous>
>::value, "tl::mix Ambiguous");

// tl::multimix
static_assert(std::is_same<
    tl::multimix<tl::set<0, A>, tl::set<1, B>, tl::set<2, C>>,
    tl::type_list<A, B, C>
>::value, "tl::multimix");

static_assert(std::is_same<
    tl::multimix<testtl1_t, testtl1_t, testtl1_t>,
    tl::type_list<tl::Ambiguous, tl::Ambiguous, tl::Ambiguous>
>::value, "tl::multimix Ambiguous");

// tl::set_all
static_assert(std::is_same<
    tl::set_all<std::index_sequence<0, 1, 2>, A>,
    tl::type_list<A, A, A>
>::value, "tl::set_all");

// Runtime tests
#include <glog/logging.h>
#include <gtest/gtest.h>

TEST(variadic_tests, seq_to_vector) {
    constexpr size_t N = 10;
    using seq_t = std::make_index_sequence<N>;
    auto vec = vector_from_sequence(seq_t());
    for(size_t i = 0; i < N; i++) {
        ASSERT_EQ(i, vec[i]);
    }
}
