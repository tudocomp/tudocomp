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

#include <tudocomp/Algorithm.hpp>

using namespace tdc;

// Base class, merely required for the Registry example
class MyAlgorithmBase : public Algorithm {
public:
    using Algorithm::Algorithm; // inherit the default constructor

    // make the destructor virtual, and define copy and move constructors
    virtual ~MyAlgorithmBase() = default;
    MyAlgorithmBase(MyAlgorithmBase const&) = default;
    MyAlgorithmBase(MyAlgorithmBase&&) = default;
    MyAlgorithmBase& operator=(MyAlgorithmBase const&) = default;
    MyAlgorithmBase& operator=(MyAlgorithmBase&&) = default;

    // mark this as having the meta type "example"
    static string_ref meta_type() { return "example"_v; };

    virtual int execute() = 0;
};

// Implement an Algorithm with a strategy
template<typename strategy_t>
class MyAlgorithm : public MyAlgorithmBase {
public:
    inline static Meta meta() {
        // define the algorithm's meta information
        Meta m(TypeDesc("example"), "my_algorithm", "An example algorithm");
        m.param("param1").primitive("default_value");
        m.param("number").primitive(147);
        m.param("strategy").strategy<strategy_t>(TypeDesc("my_strategy_t"));
        return m;
    }

    using MyAlgorithmBase::MyAlgorithmBase; // inherit the default constructor

    inline std::string param1() {
        // read param1 option as a string
        return config().param("param1").as_string();
    }

    inline virtual int execute() override {
        // read number option as an integer
        auto number = config().param("number").as_int();

        // instantiate strategy with sub environment
        strategy_t strategy(config().sub_config("strategy"));

        // use strategy to determine result
        return strategy.result(number);
    }
};

// Strategy to compute the square of the input parameter
class SquareStrategy : public Algorithm {
public:
    inline static Meta meta() {
        // define the algorithm's meta information
        Meta m(TypeDesc("my_strategy_t"), "sqr", "Computes the square");
        return m;
    }

    using Algorithm::Algorithm; // inherit the default constructor

    inline int result(int x) {
        return x * x;
    }
};

// Strategy to compute the product of the input parameter and another number
class MultiplyStrategy : public Algorithm {
public:
    inline static Meta meta() {
        // define the algorithm's meta information
        Meta m(TypeDesc("my_strategy_t"), "mul", "Computes a product");
        m.param("factor").primitive(); // no default
        return m;
    }

    using Algorithm::Algorithm; // inherit the default constructor

    inline int result(int x) {
        return x * config().param("factor").as_int();
    }
};

TEST(doc_algorithm_impl, algo_instantiate) {
    // Execute the algorithm with the square strategy
    auto algo_sqr = Algorithm::instance<MyAlgorithm<SquareStrategy>>("number=7");
    ASSERT_EQ(49, algo_sqr.execute());

    // Execute the algorithm with the multiply strategy
    auto algo_mul5 = Algorithm::instance<MyAlgorithm<MultiplyStrategy>>("number=7,strategy=mul(5)");
    ASSERT_EQ(35, algo_mul5.execute());

    // param1 was not passed and should be "default_value" for both
    ASSERT_EQ("default_value", algo_sqr.param1());
    ASSERT_EQ("default_value", algo_mul5.param1());
}

#include <tudocomp/meta/RegistryOf.hpp>

TEST(doc_algorithm_impl, algo_registry) {
    // Create a registry for algorithms of type "example"
    RegistryOf<MyAlgorithmBase> registry(TypeDesc("example"));

    // Register two specializations of the algorithm
    registry.register_algorithm<MyAlgorithm<SquareStrategy>>();
    registry.register_algorithm<MyAlgorithm<MultiplyStrategy>>();

    // Execute the algorithm with the square strategy
    auto algo_sqr = registry.select("my_algorithm(number=5, strategy=sqr)");
    ASSERT_EQ(25, algo_sqr->execute());

    // Execute the algorithm with the multiply strategy
    auto algo_mul = registry.select("my_algorithm(number=5, strategy=mul(8))");
    ASSERT_EQ(40, algo_mul->execute());
}

