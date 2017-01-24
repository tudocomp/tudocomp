#pragma once

#include <chrono>
#include <random>
#include <tudocomp/Generator.hpp>

namespace tdc {

/// \brief Generates a random string of uniformly distributed characters.
///
/// The range of generated character ASCII codes can be specified using the
/// `min` and `max` environment parameters, as well as the seed value for the
/// randomizer.
///
/// A seed of zero (default) will result in a seed obtained from the system
/// clock.
class RandomUniformGenerator : public Generator {

public:
    inline static Meta meta() {
        Meta m("generator", "random", "Generates random strings.");
        m.option("length").dynamic();
        m.option("seed").dynamic(0);
        m.option("min").dynamic('0');
        m.option("max").dynamic('9');
        return m;
    }

    inline static std::string generate(
        size_t length, size_t seed = 0, size_t min = '0', size_t max = '9') {

        if(min > max) std::swap(min, max);
        if(!seed) seed = std::chrono::system_clock::now().time_since_epoch().count();

        std::string s(length,0);
        std::default_random_engine engine(seed);
        std::uniform_int_distribution<char> dist(min, max);

        for(size_t i = 0; i < length; ++i) {
            s[i] = dist(engine);
        }

        return s;
    }

    using Generator::Generator;

    inline virtual std::string generate() override {
        return generate(
            env().option("length").as_integer(),
            env().option("seed").as_integer(),
            env().option("min").as_integer(),
            env().option("max").as_integer());
    }
};

} //ns

