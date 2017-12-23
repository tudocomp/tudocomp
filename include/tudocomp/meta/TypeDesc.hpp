#pragma once

#include <tudocomp/util/conststr.hpp>

namespace tdc {
namespace meta {

/// \brief Describes an algorithm type.
class TypeDesc {
private:
    bool m_valid;
    conststr m_name;
    const TypeDesc* m_super;

public:
    /// \brief Default constructor for an empty, invalid type.
    inline constexpr TypeDesc()
        : m_valid(false), m_name(""), m_super(nullptr) {
    }

    /// \brief Copy constructor.
    /// \param other the object to copy from
    inline constexpr TypeDesc(const TypeDesc& other)
        : m_valid(other.m_valid),
          m_name(other.m_name),
          m_super(other.m_super) {
    }

    /// \brief Constructs a type descriptor with no super type.
    ///
    /// \param name the type's name
    inline constexpr TypeDesc(conststr name)
        : m_valid(true),
          m_name(name),
          m_super(nullptr) {
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
          m_super(&super) {
    }

    /// \brief Tests whether two type descriptors are equal.
    /// \return \c true iff both types are valid and their names are equal
    inline constexpr bool operator==(const TypeDesc& other) const {
        return m_valid && other.m_valid && m_name == other.m_name;
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
        } else if(m_super) {
            return m_super->subtype_of(base);
        } else {
            return false;
        }
    }

    inline constexpr bool valid() const { return m_valid; }
    inline std::string name() const { return std::string(m_name.str()); }
    inline const TypeDesc* super() const { return m_super; }
};

constexpr TypeDesc no_type;

} //namespace meta

using TypeDesc = meta::TypeDesc;

} //namespace tdc
