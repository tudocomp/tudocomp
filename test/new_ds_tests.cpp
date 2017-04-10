#include <glog/logging.h>
#include <gtest/gtest.h>

#include <tudocomp/ds/DSManager.hpp>

#include <tudocomp/ds/providers/DivSufSort.hpp>
#include <tudocomp/ds/providers/ISAFromSA.hpp>
#include <tudocomp/ds/providers/PhiAlgorithm.hpp>
#include <tudocomp/ds/providers/PhiFromSA.hpp>

#include <tudocomp/CreateAlgorithm.hpp>

#include <memory>
#include <tuple>

using namespace tdc;

TEST(Sandbox, example) {
    // test input
    std::string input("banana\0", 7);

    // instantiate manager
    using dsmanager_t = DSManager<DivSufSort, ISAFromSA, PhiAlgorithm, PhiFromSA>;
    dsmanager_t dsman(create_env(dsmanager_t::meta()), input);
    
    // construct ISA, LCP and SA
    dsman.construct(dsid_list_t { ds::INVERSE_SUFFIX_ARRAY, ds::LCP_ARRAY, ds::SUFFIX_ARRAY });

    // get LCP array
    auto& lcp_provider = dsman.get_provider(ds::LCP_ARRAY);
}

