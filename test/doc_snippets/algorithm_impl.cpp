/**
 *
 * This file contains code snippets from the documentation as a reference.
 *
 * Please do not change this file unless you change the corresponding snippets
 * in the documentation as well!
 *
**/

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <tudocomp/tudocomp.hpp>

using namespace tdc;

// Implement an Algorithm
class MyAlgorithm : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("undisclosed", "my_algorithm", "An example algorithm");
        m.option("param1").dynamic("default_value");
        m.option("numeric").dynamic(147);
        return m;
    }

    inline MyAlgorithm(Env&& env) : Algorithm(std::move(env)) {
    }

    inline void execute() {
        auto param1 = env().option("param1").as_string();
        auto numeric = env().option("numeric").as_integer();

        ASSERT_EQ("default_value", param1);
        ASSERT_EQ(777, numeric);
    }
};

TEST(doc_algorithm_impl, algo_instantiate) {
    // Instantiate the algorithm
    auto my_algo = create_algo<MyAlgorithm>("numeric=777");

    // Execute it
    my_algo.execute();
}

