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
    Constructor<T> algorithm;
};

template<class T>
class AlgorithmRegistry {
public:
    std::vector<Algorithm<T>*> registry = {};
    template<class U>
    Algorithm<T> register_algo(Algorithm<T>* algo_loc,
                                  std::string name,
                                  std::string shortname,
                                  std::string description
                                 ) {
        registry.push_back(algo_loc);
        Algorithm<T> algo {
            name,
            shortname,
            description,
            &construct<T, U, Env&>
        };
        *algo_loc = algo;
        std::cout << algo_loc->name << std::endl;
        return algo;
    }
};

/// Declares a registry NAME for algorithms,
/// which need to inherit from Interface.
#define DECLARE_ALGO_REGISTRY(NAME, Interface) \
    extern AlgorithmRegistry<Interface> NAME;

/// Defines a registry NAME for algorithms,
/// which need to inherit from Interface.
#define DEFINE_ALGO_REGISTRY(NAME, Interface) \
    AlgorithmRegistry<Interface> NAME;

/// Registers algorithm Type in registry NAME.
/// Type needs to inherit from Interface, and the value will be paired
/// with the metadata given by name, shortname and description.
#define REGISTER_ALGO(NAME, Interface, Type, name, shortname, description) \
    extern Algorithm<Interface> NAME##_##Type;                             \
    Algorithm<Interface> NAME##_##Type =                                   \
        NAME.register_algo<Type>(&NAME##_##Type,                           \
            name, shortname, description);

}

#endif
