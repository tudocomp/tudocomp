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

using dsmanager_t = DSManager<DivSufSort, PhiAlgorithm, ISAFromSA, PhiFromSA>;

// compile-time tests
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

TEST(Sandbox, example) {
    // test input
    std::string input("banana\0", 7);

    // instantiate manager
    dsmanager_t dsman(create_env(dsmanager_t::meta()), input);

    // construct ISA, LCP and SA
    /*dsman.construct(dsid_list_t { ds::INVERSE_SUFFIX_ARRAY, ds::LCP_ARRAY, ds::SUFFIX_ARRAY });*/

    // get LCP array
    //auto& lcp_provider = dsman.get_provider(ds::LCP_ARRAY);
}

