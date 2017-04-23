#include <glog/logging.h>
#include <gtest/gtest.h>

#include <type_traits>

#include <tudocomp/ds/DSManager.hpp>

#include <tudocomp/ds/providers/DivSufSort.hpp>
#include <tudocomp/ds/providers/ISAFromSA.hpp>
#include <tudocomp/ds/providers/PhiAlgorithm.hpp>
#include <tudocomp/ds/providers/PhiFromSA.hpp>

#include <tudocomp/CreateAlgorithm.hpp>

#include <memory>
#include <tuple>

using namespace tdc;

// compile-time tests for DSManager
using dsmanager_t = DSManager<DivSufSort, PhiAlgorithm, ISAFromSA, PhiFromSA>;

static_assert(std::is_same<
        dsmanager_t::provider_type<ds::SUFFIX_ARRAY>, DivSufSort
        >::value, "Wrong provider entry for SUFFIX_ARRAY");
static_assert(std::is_same<
        dsmanager_t::provider_type<ds::INVERSE_SUFFIX_ARRAY>, ISAFromSA
        >::value, "Wrong provider entry for INVERSE_SUFFIX_ARRAY");
static_assert(std::is_same<
        dsmanager_t::provider_type<ds::PHI_ARRAY>, PhiFromSA
        >::value, "Wrong provider entry for PHI_ARRAY");
static_assert(std::is_same<
        dsmanager_t::provider_type<ds::PLCP_ARRAY>, PhiAlgorithm
        >::value, "Wrong provider entry for PLCP_ARRAY");
static_assert(std::is_same<
        dsmanager_t::provider_type<ds::LCP_ARRAY>, PhiAlgorithm
        >::value, "Wrong provider entry for LCP_ARRAY");

// compile-time tests for DSDependencyGraph
using depgraph_t = DSDependencyGraph<dsmanager_t>;

// in_degree
static_assert(0 == depgraph_t::in_degree<ds::SUFFIX_ARRAY>(),
    "Wrong in degree of SUFFIX_ARRAY node in dependency graph");
static_assert(1 == depgraph_t::in_degree<ds::INVERSE_SUFFIX_ARRAY>(),
    "Wrong in degree of INVERSE_SUFFIX_ARRAY node in dependency graph");
static_assert(1 == depgraph_t::in_degree<ds::PHI_ARRAY>(),
    "Wrong in degree of PHI_ARRAY node in dependency graph");
static_assert(1 == depgraph_t::in_degree<ds::LCP_ARRAY>(),
    "Wrong in degree of LCP_ARRAY node in dependency graph");

// cost
static_assert(0 == depgraph_t::cost<ds::SUFFIX_ARRAY>(),
    "Wrong in cost of SUFFIX_ARRAY node in dependency graph");
static_assert(1 == depgraph_t::cost<ds::INVERSE_SUFFIX_ARRAY>(),
    "Wrong in cost of INVERSE_SUFFIX_ARRAY node in dependency graph");
static_assert(1 == depgraph_t::cost<ds::PHI_ARRAY>(),
    "Wrong in cost of PHI_ARRAY node in dependency graph");
static_assert(2 == depgraph_t::cost<ds::LCP_ARRAY>(),
    "Wrong in cost of LCP_ARRAY node in dependency graph");

// construction order
static_assert(std::is_same<
    depgraph_t::construction_order<
        ds::SUFFIX_ARRAY, ds::INVERSE_SUFFIX_ARRAY, ds::LCP_ARRAY>,
    std::index_sequence<
        ds::LCP_ARRAY, ds::INVERSE_SUFFIX_ARRAY, ds::SUFFIX_ARRAY>
>::value, "Wrong construction order");

TEST(Sandbox, example) {
    // test input
    std::string input("banana\0", 7);

    // instantiate manager
    dsmanager_t dsman(create_env(dsmanager_t::meta()), input);

    // get providers (template version)
    auto& sa_provider =   dsman.get_provider<ds::SUFFIX_ARRAY>();
    auto& isa_provider =  dsman.get_provider<ds::INVERSE_SUFFIX_ARRAY>();
    auto& phi_provider =  dsman.get_provider<ds::PHI_ARRAY>();
    auto& lcp_provider =  dsman.get_provider<ds::LCP_ARRAY>();
    auto& plcp_provider = dsman.get_provider<ds::PLCP_ARRAY>();

    // make sure they are the right ones
    ASSERT_EQ("divsufsort",    std::remove_reference<decltype(sa_provider)>::type::meta().name());
    ASSERT_EQ("isa",           std::remove_reference<decltype(isa_provider)>::type::meta().name());
    ASSERT_EQ("phi",           std::remove_reference<decltype(phi_provider)>::type::meta().name());
    ASSERT_EQ("phi_algorithm", std::remove_reference<decltype(lcp_provider)>::type::meta().name());
    ASSERT_EQ("phi_algorithm", std::remove_reference<decltype(plcp_provider)>::type::meta().name());

    // get abstract providers (runtime version)
    auto p_sa_provider   = dsman.abstract_provider(ds::SUFFIX_ARRAY);
    auto p_isa_provider  = dsman.abstract_provider(ds::INVERSE_SUFFIX_ARRAY);
    auto p_phi_provider  = dsman.abstract_provider(ds::PHI_ARRAY);
    auto p_lcp_provider  = dsman.abstract_provider(ds::LCP_ARRAY);
    auto p_plcp_provider = dsman.abstract_provider(ds::PLCP_ARRAY);

    // make sure they are the right ones
    ASSERT_EQ(p_sa_provider.get(),   static_cast<DSProvider*>(&sa_provider));
    ASSERT_EQ(p_isa_provider.get(),  static_cast<DSProvider*>(&isa_provider));
    ASSERT_EQ(p_phi_provider.get(),  static_cast<DSProvider*>(&phi_provider));
    ASSERT_EQ(p_lcp_provider.get(),  static_cast<DSProvider*>(&lcp_provider));
    ASSERT_EQ(p_plcp_provider.get(), static_cast<DSProvider*>(&plcp_provider));

    // create dependency graph for some data structures


    // construct ISA, LCP and SA
    /*{
        dsman.construct<
            ds::INVERSE_SUFFIX_ARRAY,
            ds::LCP_ARRAY,
            ds::SUFFIX_ARRAY>();
    }*/

    // get LCP array
    //auto& lcp_provider = dsman.get_provider(ds::LCP_ARRAY);
}

