#pragma once

#include <sstream>
#include <tudocomp/util/conststr.hpp>

namespace tdc {
namespace meta {

/// \brief Describes an algorithm type.
class TypeDesc {
private:
    static constexpr size_t MAX_SUPERTYPES = 16ULL;

    bool m_valid;
    conststr m_name;

    conststr m_supers[MAX_SUPERTYPES];
    size_t m_num_supers;

public:
    /// \brief Default constructor for an empty, invalid type.
    inline constexpr TypeDesc()
        : m_valid(false), m_name(""), m_num_supers(0) {
    }

    /// \brief Copy constructor.
    /// \param other the object to copy from
    inline constexpr TypeDesc(const TypeDesc& other)
        : m_valid(other.m_valid),
          m_name(other.m_name),
          m_num_supers(other.m_num_supers) {

        for(size_t i = 0; i < m_num_supers; i++) {
            m_supers[i] = other.m_supers[i];
        }
    }

    /// \brief Constructs a type descriptor with no super type.
    ///
    /// \param name the type's name
    inline constexpr TypeDesc(conststr name)
        : m_valid(true),
          m_name(name),
          m_num_supers(0) {
    }

    /// \brief Constructs a type descriptor with a super type.
    ///
    /// The semantics are that this type can be morphed to the super type
    /// when necessary, ie., algorithms of this type can be used when
    /// a algorithm of the super type is expected.
    ///
    /// \param name the type's name
    /// \param super the super type
    inline constexpr TypeDesc(conststr name, const TypeDesc& super)
        : m_valid(true),
          m_name(name),
          m_num_supers(super.m_num_supers + 1) {

        m_supers[0] = super.m_name;
        for(size_t i = 0; i < super.m_num_supers; i++) {
            m_supers[i+1] = super.m_supers[i];
        }
    }

    /// \brief Tests whether two type descriptors are equal.
    /// \return \c true iff both types are valid and their names are equal
    inline constexpr bool operator==(const TypeDesc& other) const {
        if(m_valid &&
            other.m_valid &&
            m_num_supers == other.m_num_supers &&
            m_name == other.m_name) {

            // also compare super chain
            for(size_t i = 0; i < m_num_supers; i++) {
                if(m_supers[i] != other.m_supers[i]) return false;
            }

            return true;
        } else {
            return false;
        }
    }

    /// \brief Tests whether two type descriptors are unequal.
    /// \return \c true iff the types are not equal
    inline constexpr bool operator!=(const TypeDesc& other) const {
        return !(*this == other);
    }

    /// \brief Tests whether this is a subtype of a given base type.
    ///
    /// \param base the base type in question
    /// \return \c true iff this type or any super type of this type equals
    ///         the base type
    inline constexpr bool subtype_of(const TypeDesc& base) const {
        if(*this == base) {
            return true;
        } else if(m_valid && base.m_valid) {
            for(size_t i = 0; i < m_num_supers; i++) {
                if(m_supers[i] == base.m_name) return true;
            }
            return false;
        } else {
            return false;
        }
    }

    inline constexpr bool valid() const {
        return m_valid;
    }

    inline constexpr TypeDesc super() const {
        if(m_num_supers > 0) {
            TypeDesc super(m_supers[0]);

            super.m_num_supers = m_num_supers - 1;
            for(size_t i = 1; i < m_num_supers; i++) {
                super.m_supers[i-1] = m_supers[i];
            }

            return super;
        } else {
            return TypeDesc();
        }
    }

    inline std::string name() const {
        return std::string(m_name.str());
    }

    inline std::string canonical_id() const {
        std::stringstream id;
        id << m_name.str();
        for(size_t i = 0; i < m_num_supers; i++) {
            id << ':' << m_supers[i].str();
        }
        return id.str();
    }
};

constexpr TypeDesc no_type;

} //namespace meta

using TypeDesc = meta::TypeDesc;

} //namespace tdc

#include <functional>

namespace std {
    template<> struct hash<tdc::TypeDesc> {
        size_t operator()(tdc::TypeDesc const& td) const noexcept {
            return std::hash<std::string>{}(td.canonical_id());
        }
    };
}

