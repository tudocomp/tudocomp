#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include <tudocomp/meta/AlgorithmConfig.hpp>
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
    using ctor_t = std::function<std::unique_ptr<T>(AlgorithmConfig&&)>;

    TypeDesc m_root_type;
    AlgorithmLib m_lib;
    std::unordered_map<std::string, ctor_t> m_reg;

public:
    inline Registry(const TypeDesc& root_type) : m_root_type(root_type) {
    }

    template<typename Algo>
    inline void register_algorithm() {
        auto meta = Algo::meta();
        
        if(!meta.decl()->type().subtype_of(m_root_type)) {
            throw RegistryError(std::string(
                "trying to register algorithm of type ") +
                meta.decl()->type().name() +
                std::string(", expected ") +
                m_root_type.name());
        }

        add_to_lib(m_lib, meta);

        auto sig = meta.signature()->str();
        auto it = m_reg.find(sig);
        if(it == m_reg.end()) {
            m_reg.emplace(sig, [](AlgorithmConfig&& cfg) {
                return std::make_unique<Algo>(std::move(cfg));
            });
        } else {
            throw RegistryError(std::string("already registered: ") + sig);
        }
    }

    inline std::unique_ptr<T> select(ast::NodePtr<ast::Object> obj) {
        auto lib_entry = m_lib.find(obj->name());
        if(lib_entry == m_lib.end()) {
            throw RegistryError(
                std::string("unknown algorithm: ") + obj->name());
        }

        auto decl = lib_entry->second;
        DLOG(INFO) << "found declaration: " << decl->name()
                   << " (" << decl->type().name() << ")";

        auto cfg = AlgorithmConfig(*decl, obj, m_lib);
        DLOG(INFO) << "config: " << cfg.str();

        auto sig = cfg.signature();
        DLOG(INFO) << "signature: " << sig->str();

        auto reg_entry = m_reg.find(sig->str());
        if(reg_entry == m_reg.end()) {
            throw RegistryError(
                std::string("unregistered instance: ") + sig->str());
        }

        return (reg_entry->second)(std::move(cfg));
    }

    inline std::unique_ptr<T> select(const std::string& str) {
        auto obj = ast::convert<ast::Object>(ast::Parser::parse(str));
        DLOG(INFO) << "parsed AST: " << obj->str();
        return select(obj);
    }
};

} //namespace meta

template<typename T>
using Registry = meta::Registry<T>;

} //namespace tdc
