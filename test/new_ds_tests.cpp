#include <glog/logging.h>
#include <gtest/gtest.h>

#include <tudocomp/ds/DSManager.hpp>

#include <tudocomp/ds/providers/DivSufSort.hpp>
#include <tudocomp/ds/providers/ISAFromSA.hpp>
#include <tudocomp/ds/providers/PhiAlgorithm.hpp>
#include <tudocomp/ds/providers/PhiFromSA.hpp>

#include <tudocomp/CreateAlgorithm.hpp>

using namespace tdc;

TEST(Sandbox, example) {
    // test input
    std::string input("banana\0", 7);

    // instantiate manager
    DSManager dsman(create_env(DSManager::meta()), input);
    
    // create and register algorithms (TODO: automate)
    DivSufSort divsufsort(create_env(DivSufSort::meta()));
    dsman.register_provider(divsufsort);

    ISAFromSA isa_from_sa(create_env(ISAFromSA::meta()));
    dsman.register_provider(isa_from_sa);

    PhiAlgorithm phi_algo(create_env(PhiAlgorithm::meta()));
    dsman.register_provider(phi_algo);

    PhiFromSA phi_from_sa(create_env(PhiFromSA::meta()));
    dsman.register_provider(phi_from_sa);

    // construct ISA, LCP and SA
    dsman.construct(dsid_list_t { ds::INVERSE_SUFFIX_ARRAY, ds::LCP_ARRAY, ds::SUFFIX_ARRAY });
}

