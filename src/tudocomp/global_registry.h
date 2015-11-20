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
#include <functional>
#include <unordered_map>
#include <tuple>
#include <type_traits>

#include "boost/utility/string_ref.hpp"
#include "glog/logging.h"

#include "rule.h"
#include "rules.h"
#include "tudocomp_env.h"

namespace tudocomp {

// implementation details, users never invoke these directly
namespace tuple_call
{
    template <typename F, typename Tuple, bool Done, int Total, int... N>
    struct call_impl
    {
        static void call(F f, Tuple && t)
        {
            call_impl<F, Tuple, Total == 1 + sizeof...(N), Total, N..., sizeof...(N)>::call(f, std::forward<Tuple>(t));
        }
    };

    template <typename F, typename Tuple, int Total, int... N>
    struct call_impl<F, Tuple, true, Total, N...>
    {
        static void call(F f, Tuple && t)
        {
            f(std::get<N>(std::forward<Tuple>(t))...);
        }
    };
}

// user invokes this
template <typename F, typename Tuple>
void call(F f, Tuple && t)
{
    typedef typename std::decay<Tuple>::type ttype;
    tuple_call::call_impl<F, Tuple, 0 == std::tuple_size<ttype>::value, std::tuple_size<ttype>::value>::call(f, std::forward<Tuple>(t));
}

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
    std::function<T*(Env&)> algorithm;
};

template<class T>
using Registry = std::vector<Algorithm<T>>;

template<class T>
class AlgorithmRegistry;

template<class T, class SubT, class ... SubAlgos>
struct AlgorithmBuilder {
    Env& m_env;
    Algorithm<T> info;
    Registry<T>& registry;
    std::tuple<AlgorithmRegistry<SubAlgos>...> sub_algos;

    template<class U>
    AlgorithmBuilder<T, SubT, SubAlgos..., U>
    with_sub_algos(std::function<void (AlgorithmRegistry<U>&)> f);

    inline void do_register();
};

template<class T>
class AlgorithmRegistry {
    Env& m_env;
public:
    inline AlgorithmRegistry(Env& env): m_env(env) { }

    std::vector<Algorithm<T>> registry = {};

    template<class U>
    AlgorithmBuilder<T, U> with_info(std::string name,
                                     std::string shortname,
                                     std::string description) {
        Algorithm<T> algo {
            name,
            shortname,
            description,
            nullptr
        };
        AlgorithmBuilder<T, U> builder {
            m_env,
            algo,
            registry,
            {}
        };
        //
        return builder;
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

template<class T, class SubT, class ... SubAlgos>
template<class U>
AlgorithmBuilder<T, SubT, SubAlgos..., U> AlgorithmBuilder<T, SubT, SubAlgos...>
        ::with_sub_algos(std::function<void(AlgorithmRegistry<U>&)> f) {
    AlgorithmRegistry<U> reg(m_env);
    f(reg);
    return {
        m_env,
        info,
        registry,
        std::tuple_cat(sub_algos, std::tuple<AlgorithmRegistry<U>>{reg})
    };
}

template<class T, class SubT, class ... SubAlgos>
inline void AlgorithmBuilder<T, SubT, SubAlgos...>::do_register() {
    info.algorithm = [=](Env& env) -> T* {
        SubT* r;
        call(
            [=, &env, &r](AlgorithmRegistry<SubAlgos> ... args) {
                r = new SubT(env, args.registry[0].algorithm(env)...);
            },
            sub_algos
        );
        return r;
    };
    registry.push_back(info);
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
