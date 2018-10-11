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
        Meta m(Generator::type_desc(), "random",
            "Generates a random string from a given symbol range with uniform "
            "symbol distribution."
        );
        m.param("n", "The length of the generated string.").primitive();
        m.param("seed").primitive(0);
        m.param("min", "The ASCII code of the first symbol in the alphabet.")
            .primitive('0');
        m.param("max", "The ASCII code of the last symbol in the alphabet.")
            .primitive('9');
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
            config().param("n").as_uint(),
            config().param("seed").as_uint(),
            config().param("min").as_uint(),
            config().param("max").as_uint());
    }
};

} //ns

