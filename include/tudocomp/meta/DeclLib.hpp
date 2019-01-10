#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <tudocomp/meta/Decl.hpp>

namespace tdc {
namespace meta {

/// \brief Error type for library related errors.
class LibError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/// \brief A library of algorithm declarations.
class DeclLib {
private:
    using inner_map_t = std::unordered_map<
        std::string, std::shared_ptr<const Decl>>;

    std::unordered_map<std::string, inner_map_t> m_lib;

public:
    /// \brief Main Constructor.
    inline DeclLib() {
    }

private:
    inline void insert_recursively(
        std::shared_ptr<const Decl> decl,
        const TypeDesc& type) {

        // recursively add for super types
        // (we do not want to continue if that fails)
        {
            auto super = type.super();
            if(super.valid()) insert_recursively(decl, super);
        }

        // first, get the sublibrary for the given type
        decltype(m_lib)::iterator it_for_type;
        {
            auto it = m_lib.find(type.name());
            if(it != m_lib.end()) {
                it_for_type = it;
            } else {
                it_for_type = m_lib.emplace(
                    type.name(), inner_map_t()).first; // grrrrr....
            }
        }

        // now attempt to enter the declaration
        auto& lib_for_type = it_for_type->second;
        auto it = lib_for_type.find(decl->name());
        if(it != lib_for_type.end()) {
            auto other_decl = it->second;
            if(other_decl->type() != decl->type()) {
                throw LibError(std::string("conflict for '") + decl->name()
                    + "' - one is of type " + decl->type().name()
                    + ", the other is of type " + other_decl->type().name()
                    + ", both inherit from " + type.name());
            }
        } else {
            lib_for_type.emplace(decl->name(), decl);
        }
    }

public:
    /// \brief Inserts a declaration into the library.
    ///
    /// If a declaration of the same name and type already exists, nothing is
    /// added. If the declaration's type inherits another type and a
    /// different name has already been inserted for the supertype, an error
    /// is thrown.
    ///
    /// \param decl the declaration to insert
    inline void insert(std::shared_ptr<const Decl> decl) {
        insert_recursively(decl, decl->type());
    }

private:
    inline std::shared_ptr<const Decl> find_for_type(
        const std::string& name,
        const std::string& type) const {

        auto it = m_lib.find(type);
        if(it != m_lib.end()) {
            auto& lib_for_type = it->second;
            auto inner_it = lib_for_type.find(name);
            if(inner_it != lib_for_type.end()) {
                return inner_it->second;
            }
        }

        return std::shared_ptr<const Decl>();
    }

public:
    /// \brief Finds a declaration of the given name and type in the library.
    /// \param name the name of the declaration to find
    /// \param type the type of the declaration to find
    /// \param include_subtypes if \c true, declarations whose type inherit
    ///                         the specified type are also considered
    /// \return the found declaration, or an empty pointer if none was found
    inline std::shared_ptr<const Decl> find(
        const std::string& name,
        const std::string& type,
        bool include_subtypes = true) const {

        std::shared_ptr<const Decl> result = find_for_type(name, type);
        if(result && !include_subtypes && result->type().name() != type) {
            return std::shared_ptr<const Decl>(); // "null"
        } else {
            return result;
        }
    }

    /// \brief Finds a declaration of the given name and type in the library.
    /// \param name the name of the declaration to find
    /// \param type the type of the declaration to find
    /// \param include_subtypes if \c true, declarations whose type inherit
    ///                         the specified type are also considered
    /// \return the found declaration, or an empty pointer if none was found
    inline std::shared_ptr<const Decl> find(
        const std::string& name,
        const TypeDesc& type,
        bool include_subtypes = true) const {

        return find(name, type.name(), include_subtypes);
    }

    /// \brief Finds all declarations matching the specified type.
    /// \param type the type of declarations to retrieve
    /// \param include_subtypes if \c true, declarations whose type inherit
    /// \return a list of all found declarations in no specific order
    inline std::vector<std::shared_ptr<const Decl>> type_entries(
        const std::string& type,
        bool include_subtypes = true) const {

        std::vector<std::shared_ptr<const Decl>> result;
        auto it = m_lib.find(type);
        if(it != m_lib.end()) {
            auto& lib_for_type = it->second;
            for(auto e : lib_for_type) {
                auto decl = e.second;
                if(include_subtypes || decl->type().name() == type) {
                    result.emplace_back(decl);
                }
            }
        }
        return result;
    }

    /// \brief Finds all declarations with the specified name, independent of
    ///        their type.
    /// \param name the name of the declarations to retrieve
    /// \return a list of all found declarations in no specific order
    inline std::vector<std::shared_ptr<const Decl>> name_entries(
        const std::string& name) {

        std::vector<std::shared_ptr<const Decl>> result;
        for(auto outer : m_lib) {
            auto& lib_for_type = outer.second;
            auto it = lib_for_type.find(name);
            if(it != lib_for_type.end()) {
                result.emplace_back(it->second);
            }
        }
        return result;
    }

    /// \brief Returns a list of all entered declarations.
    /// \return a list of all entered declarations in no specific order
    inline std::vector<std::shared_ptr<const Decl>> entries() const {
        std::vector<std::shared_ptr<const Decl>> e;
        for(auto outer : m_lib) {
            for(auto inner : outer.second) {
                e.emplace_back(inner.second);
            }
        }
        return e;
    }
};

inline DeclLib operator+(const DeclLib& a, const DeclLib& b) {
    DeclLib merged = a;
    for(auto e : b.entries()) merged.insert(e);
    return merged;
}

}} //ns
