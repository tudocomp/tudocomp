#pragma once

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
        m.option("min").dynamic("48");
        m.option("max").dynamic("57");
        m.option("seed").dynamic("0");
        return m;
    }

    using Generator::Generator;

    inline virtual std::string generate() override {
        size_t length = env().option("length").as_integer();
        size_t min = env().option("min").as_integer();
        size_t max = env().option("max").as_integer();
        size_t seed = env().option("seed").as_integer();

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
};

} //ns

