#include <glog/logging.h>
#include <gtest/gtest.h>

#include <tudocomp/util/type_list.hpp>

using namespace tdc;

// providers
struct A {
    using provides = tl::set_all<std::index_sequence<0, 2>, A>;

    static constexpr int test_id = 0xA;
};

struct B {
    using provides = tl::set<3, B>;

    static constexpr int test_id = 0xB;
};

struct C {
    using provides = tl::set_all<std::index_sequence<1, 4>, C>;

    static constexpr int test_id = 0xC;
};

// manager
using dsid_t = size_t;

template<typename... Providers>
struct manager {
    using providers = tl::multimix<typename Providers::provides...>;

    template<dsid_t Id>
    using get_provider = tl::get<Id, providers>;
};

// instance
using manager_t = manager<B, C, A>;

// test mapping at compile time
static_assert(manager_t::get_provider<0>::test_id == 0xA, "something's wrong");
static_assert(manager_t::get_provider<1>::test_id == 0xC, "something's wrong");
static_assert(manager_t::get_provider<2>::test_id == 0xA, "something's wrong");
static_assert(manager_t::get_provider<3>::test_id == 0xB, "something's wrong");
static_assert(manager_t::get_provider<4>::test_id == 0xC, "something's wrong");
