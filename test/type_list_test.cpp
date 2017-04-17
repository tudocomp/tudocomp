#include <glog/logging.h>
#include <gtest/gtest.h>

#include <tudocomp/util/type_list.hpp>

/*
    TODO:
    - allow "mix" of lists of different sizes
    - sugar syntax for "provides" lists below
      - type_list::set<I, T>
      - creates a type list of (I-1) times None and the I-th type is T
      - can be combined using mix in a function type_list::multiset<I..., T...>
    - retain "source index" in a mixed type list
*/

// providers
struct A {
    using provides = tdc::type_list::type_list<
        /*0*/ A,
        /*1*/ tdc::type_list::None,
        /*2*/ A,
        /*3*/ tdc::type_list::None,
        /*4*/ tdc::type_list::None
    >;

    static constexpr int test_id = 0xA;
};

struct B {
    using provides = tdc::type_list::type_list<
        /*0*/ tdc::type_list::None,
        /*1*/ tdc::type_list::None,
        /*2*/ tdc::type_list::None,
        /*3*/ B,
        /*4*/ tdc::type_list::None
    >;

    static constexpr int test_id = 0xB;
};

struct C {
    using provides = tdc::type_list::type_list<
        /*0*/ tdc::type_list::None,
        /*1*/ C,
        /*2*/ tdc::type_list::None,
        /*3*/ tdc::type_list::None,
        /*4*/ C
    >;

    static constexpr int test_id = 0xC;
};

// manager
using dsid_t = size_t;

template<typename... Providers>
struct manager {
    using providers = typename tdc::type_list::multimix<
        typename Providers::provides...>::list;

    template<dsid_t Id>
    using get_provider = typename tdc::type_list::get_type<Id, providers>::type;
};

// instance
using manager_t = manager<B, C, A>;

// test mapping at compile time
static_assert(manager_t::get_provider<0>::test_id == 0xA, "something's wrong");
static_assert(manager_t::get_provider<1>::test_id == 0xC, "something's wrong");
static_assert(manager_t::get_provider<2>::test_id == 0xA, "something's wrong");
static_assert(manager_t::get_provider<3>::test_id == 0xB, "something's wrong");
static_assert(manager_t::get_provider<4>::test_id == 0xC, "something's wrong");
