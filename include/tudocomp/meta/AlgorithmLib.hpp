#pragma once

#include <forward_list>
#include <string>
#include <unordered_map>
#include <tudocomp/meta/AlgorithmDecl.hpp>

namespace tdc {
namespace meta {

/// \brief Error type for library related errors.
class LibError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/// \brief A library of algorithm declarations.
class AlgorithmLib {
private:
    using inner_map_t = std::unordered_map<
        std::string, std::shared_ptr<const AlgorithmDecl>>;

    std::unordered_map<std::string, inner_map_t> m_lib;

public:
    /// \brief Main Constructor.
    inline AlgorithmLib() {
    }

    /// \brief Copy constructor.
    /// \param other the object to copy
    inline AlgorithmLib(const AlgorithmLib& other) : m_lib(other.m_lib) {
    }

    /// \brief Move constructor.
    /// \param other the object to move
    inline AlgorithmLib(AlgorithmLib&& other) : m_lib(std::move(other.m_lib)) {
    }

private:
    inline void insert_for_type(
        std::shared_ptr<const AlgorithmDecl> decl,
        const TypeDesc& type) {

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
    inline void insert(std::shared_ptr<const AlgorithmDecl> decl) {
        const TypeDesc* typep = &decl->type();
        do {
            insert_for_type(decl, *typep);
            typep = typep->super();
        } while(typep);
    }

private:
    inline std::shared_ptr<const AlgorithmDecl> find_for_type(
        const std::string& name,
        const TypeDesc& type) const {

        auto it = m_lib.find(type.name());
        if(it != m_lib.end()) {
            auto& lib_for_type = it->second;
            auto inner_it = lib_for_type.find(name);
            if(inner_it != lib_for_type.end()) {
                return inner_it->second;
            }
        }

        return std::shared_ptr<const AlgorithmDecl>();
    }

public:
    /// \brief Finds a declaration of the given name and type in the library.
    /// \param name the name of the declaration to find
    /// \param type the type of the declaration to find
    /// \param include_subtypes if \c true, declarations whose type inherit
    ///                         the specified type are also considered
    /// \return the found declaration, or an empty pointer if none was found
    inline std::shared_ptr<const AlgorithmDecl> find(
        const std::string& name,
        const TypeDesc& type,
        bool include_subtypes = true) const {

        std::shared_ptr<const AlgorithmDecl> result = find_for_type(name, type);
        if(result && !include_subtypes && result->type() != type) {
            return std::shared_ptr<const AlgorithmDecl>(); // "null"
        } else {
            return result;
        }
    }

    /// \brief Returns a list of all entered declarations.
    /// \return a list of all entered declarations with no specific order
    inline std::forward_list<std::shared_ptr<const AlgorithmDecl>> entries() const {
        std::forward_list<std::shared_ptr<const AlgorithmDecl>> e;
        for(auto outer : m_lib) {
            for(auto inner : outer.second) {
                e.emplace_front(inner.second);
            }
        }
        return e;
    }
};

inline AlgorithmLib operator+(const AlgorithmLib& a, const AlgorithmLib& b) {
    AlgorithmLib merged = a;
    for(auto e : b.entries()) merged.insert(e);
    return merged;
}

}} //ns