#ifndef TUDOCOMP_REGISTRY_H
#define TUDOCOMP_REGISTRY_H

#include <vector>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <istream>
#include <streambuf>
#include <sstream>
#include <string>
#include <map>
#include <unordered_map>

#include "boost/utility/string_ref.hpp"
#include "glog/logging.h"

#include "rule.h"
#include "rules.h"
#include "tudocomp_env.h"

namespace tudocomp {

template<class T>
using Constructor = T* (*)(tudocomp::Env&);

template<class Base, class T, class ... Args>
Base* construct(Args ... args) {
    return new T(args...);
}

template<class T>
struct Algorithm {
    /// Human readable name
    std::string name;
    /// Used as id string for command line and output filenames
    std::string shortname;
    /// Description text
    std::string description;
    /// Algorithm
    T* algorithm;
};

template<class T>
class AlgorithmRegistry {
    Env& m_env;
public:
    inline AlgorithmRegistry(Env& env): m_env(env) { }

    std::vector<Algorithm<T>> registry = {};

    template<class U>
    U* register_algo(std::string name,
                     std::string shortname,
                     std::string description) {
        U* a = new U(m_env);
        Algorithm<T> algo {
            name,
            shortname,
            description,
            a
        };
        registry.push_back(algo);
        return a;
    }

    inline Algorithm<T>* findByShortname(boost::string_ref s) {
        for (auto& x: registry) {
            if (x.shortname == s) {
                return &x;
            }
        }
        return nullptr;
    }
};

/// Declares a registry NAME for algorithms,
/// which need to inherit from Interface.
#define DECLARE_ALGO_REGISTRY(NAME, Interface)

/// Defines a registry NAME for algorithms,
/// which need to inherit from Interface.
#define DEFINE_ALGO_REGISTRY(NAME, Interface)

/// Registers algorithm Type in registry NAME.
/// Type needs to inherit from Interface, and the value will be paired
/// with the metadata given by name, shortname and description.
#define REGISTER_ALGO(NAME, Interface, Type, name, shortname, description)

}

#endif
