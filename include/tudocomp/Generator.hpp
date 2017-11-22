#pragma once

#include <functional>
#include <memory>

#include <tudocomp/pre_header/Registry.hpp>
#include <tudocomp/pre_header/Env.hpp>
#include <tudocomp/Algorithm.hpp>

namespace tdc {

/// \brief Base for string generators.
class Generator : public Algorithm {
public:
    static string_ref meta_type() { return "generator"_v; };

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

