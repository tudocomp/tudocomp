#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include <tudocomp/meta/Meta.hpp>

namespace tdc {
namespace meta {

/// \brief Error type for registry related errors.
class RegistryError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

template<typename T>
class Registry {
private:
    using ctor_t = std::function<std::unique_ptr<T>()>;

    AlgorithmLib m_lib;
    std::unordered_map<std::string, ctor_t> m_reg;

public:
    template<typename Algo>
    inline void register_algorithm() {
        auto meta = Algo::meta();
        add_to_lib(m_lib, meta);

        auto sig = meta.signature()->str();
        auto it = m_reg.find(sig);
        if(it == m_reg.end()) {
            m_reg.emplace(sig, []() {
                return std::make_unique<Algo>();
            });
        } else {
            throw RegistryError(std::string("already registered: ") + sig);
        }
    }

    inline std::unique_ptr<T> select(const std::string& str) {
        auto ast = ast::Parser::parse(str);
        DLOG(INFO) << "parsed AST: " << ast->str();

        //TODO: poll AlgorithmLib, create config, get signature, lookup

        return std::unique_ptr<T>(); // TODO: implement
    }

    //TODO: only for debugging
    inline const AlgorithmLib& lib() const { return m_lib; }
};

} //namespace meta

template<typename T>
using Registry = meta::Registry<T>;

} //namespace tdc
