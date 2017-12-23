#pragma once

#include <string>
#include <tudocomp/Algorithm.hpp>

namespace tdc {

/// \brief Base for string generators.
class Generator : public Algorithm {
public:
    static inline constexpr TypeDesc type_desc() {
        return TypeDesc("generator");
    }
virtual ~Generator() = default;
    Generator(Generator const&) = default;
    Generator(Generator&&) = default;
    Generator& operator=(Generator const&) = default;
    Generator& operator=(Generator&&) = default;

    using Algorithm::Algorithm;

    /// \brief Generates a string based on the environment settings.
    /// \return the generated string.
    virtual std::string generate() = 0;
};

}

