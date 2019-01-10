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

using namespace tdc;

class MyGenerator : public Generator {
public:
    inline static Meta meta() {
        Meta m(Generator::type_desc(), "my_generator", "An example generator");
        m.param("length").primitive();
        m.param("char").primitive('a');
        return m;
    }

    inline static std::string generate(size_t length, char c) {
        return std::string(length, c);
    }

    using Generator::Generator;

    inline virtual std::string generate() override {
        return generate(
            config().param("length").as_uint(),
            config().param("char").as_int());
    }
};

TEST(doc_generator_impl, test) {
    auto generator = Algorithm::instance<MyGenerator>("length=7, char=64");
    ASSERT_EQ("@@@@@@@", generator->generate());
}

