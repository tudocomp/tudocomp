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

#include <tudocomp/Generator.hpp>
#include <tudocomp/CreateAlgorithm.hpp>

using namespace tdc;

class MyGenerator : public Generator {
public:
    inline static Meta meta() {
        Meta m("generator", "my_compressor", "An example compressor");
        m.option("length").dynamic();
        m.option("char").dynamic('a');
        return m;
    }

    inline static std::string generate(size_t length, char c) {
        return std::string(length, c);
    }

    using Generator::Generator;

    inline virtual std::string generate() override {
        return generate(
            env().option("length").as_integer(),
            env().option("char").as_integer());
    }
};

TEST(doc_generator_impl, test) {
    auto generator = create_algo<MyGenerator>("length=7, char=64");
    ASSERT_EQ("@@@@@@@", generator.generate());
}

