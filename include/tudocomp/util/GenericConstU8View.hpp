#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <iomanip>
#include <cstring>
#include <glog/logging.h>
#include <functional>

#include <tudocomp/pre_header/GenericViewBase.hpp>
#include <tudocomp/util/GenericConstView.hpp>

namespace tdc {

/// A const view into a slice of memory.
///
/// This is an abstraction around a pointer to `T` and a size,
/// and represents N elements of memory starting at that pointer.
///
/// Creating/Copying/Modifying a View will not copy any of the data it points at.
/// Its API mostly mirrors that of a `std::vector<T>`.
template<>
class ConstGenericView<uint8_t>: GenericViewBase<uint8_t, const uint8_t*> {
    using Super = GenericViewBase<uint8_t, const uint8_t*>;
    inline ConstGenericView(const Super& other): Super::GenericViewBase(other) {}
    friend class GenericView<uint8_t>;
public:
    using value_type             = typename Super::value_type;
    using const_reference        = typename Super::const_reference;
    using const_pointer          = typename Super::const_pointer;
    using const_iterator         = typename Super::const_iterator;
    using const_reverse_iterator = typename Super::const_reverse_iterator;
    using difference_type        = typename Super::difference_type;
    using size_type              = typename Super::size_type;

    static const size_type npos  = Super::npos;

    /// Construct a empty View
    inline ConstGenericView(): Super::GenericViewBase() {}

    /// Construct a View pointing at `len` elements starting from `data`
    inline ConstGenericView(const uint8_t* data, size_t len): Super::GenericViewBase(data, len) {}

    /// Construct a View as a copy of `other`
    inline ConstGenericView(const ConstGenericView& other):
        Super::GenericViewBase(other) {}

    /// Construct a View as a copy of `other`
    inline ConstGenericView(const GenericView<uint8_t>& other):
        ConstGenericView(other.data(), other.size()) {}

    /// Construct a View pointing at the contents of a vector
    inline ConstGenericView(const std::vector<uint8_t>& other):
        Super::GenericViewBase(other.data(), other.size()) {}

    /// Construct a vector with the contents of this View
    using Super::operator std::vector<uint8_t>;

    using Super::begin;
    using Super::end;
    using Super::rbegin;
    using Super::rend;
    using Super::cbegin;
    using Super::cend;
    using Super::crbegin;
    using Super::crend;

    using Super::size;
    using Super::max_size;
    using Super::empty;
    using Super::operator[];
    using Super::at;
    using Super::front;
    using Super::back;
    using Super::data;
    using Super::pop_back;
    using Super::pop_front;

    inline void swap(ConstGenericView& other) {
        Super::swap(other);
    }

    using Super::clear;

    inline ConstGenericView substr(size_type pos, size_type len = npos) const {
        return ConstGenericView(Super::substr(pos, len));
    }

    inline ConstGenericView slice(size_type from, size_type to = npos) const {
        return ConstGenericView(Super::slice(from, to));
    }

    using Super::remove_prefix;
    using Super::remove_suffix;
    inline bool starts_with(const uint8_t& other) const {
        return Super::starts_with(other);
    }
    inline bool starts_with(const ConstGenericView<uint8_t> other) const {
        return Super::starts_with(other);
    }
    inline bool ends_with(const uint8_t& other) const {
        return Super::ends_with(other);
    }
    inline bool ends_with(const ConstGenericView<uint8_t> other) const {
        return Super::ends_with(other);
    }

    template<class U>
    friend void swap(ConstGenericView<U>& lhs, ConstGenericView<U>& rhs);

    friend bool operator==(const ConstGenericView<uint8_t>& lhs, const ConstGenericView<uint8_t>& rhs);
    friend bool operator!=(const ConstGenericView<uint8_t>& lhs, const ConstGenericView<uint8_t>& rhs);
    friend bool operator<(const ConstGenericView<uint8_t>& lhs, const ConstGenericView<uint8_t>& rhs);
    friend bool operator<=(const ConstGenericView<uint8_t>& lhs, const ConstGenericView<uint8_t>& rhs);
    friend bool operator>(const ConstGenericView<uint8_t>& lhs, const ConstGenericView<uint8_t>& rhs);
    friend bool operator>=(const ConstGenericView<uint8_t>& lhs, const ConstGenericView<uint8_t>& rhs);

    // string extensions

    /// Construct a View pointing at the contents of a string
    inline ConstGenericView(const std::string& other):
        ConstGenericView((const uint8_t*) other.data(), other.size()) {}

    /// Construct a View pointing at a C-style null terminated string.
    ///
    /// Can be used to construct from a string literal
    inline ConstGenericView(const char* other):
        ConstGenericView((const uint8_t*) other, strlen(other)) {}

    /// Construct a View pointing at `len` elements starting from `data`
    inline ConstGenericView(const char* data, size_t len):
        ConstGenericView((const uint8_t*) data, len) {}

    /// Construct a string with the contents of this View
    inline operator std::string() const {
        return std::string(cbegin(), cend());
    }
};

inline bool operator==(const ConstGenericView<uint8_t>& lhs, const ConstGenericView<uint8_t>& rhs) {
    return ConstGenericView<uint8_t>::Super::op_eq(lhs, rhs);
}

inline bool operator!=(const ConstGenericView<uint8_t>& lhs, const ConstGenericView<uint8_t>& rhs) {
    return ConstGenericView<uint8_t>::Super::op_not_eq(lhs, rhs);
}

inline bool operator<(const ConstGenericView<uint8_t>& lhs, const ConstGenericView<uint8_t>& rhs) {
    return ConstGenericView<uint8_t>::Super::op_less(lhs, rhs);
}

inline bool operator<=(const ConstGenericView<uint8_t>& lhs, const ConstGenericView<uint8_t>& rhs) {
    return ConstGenericView<uint8_t>::Super::op_less_eq(lhs, rhs);
}

inline bool operator>(const ConstGenericView<uint8_t>& lhs, const ConstGenericView<uint8_t>& rhs) {
    return ConstGenericView<uint8_t>::Super::op_greater(lhs, rhs);
}

inline bool operator>=(const ConstGenericView<uint8_t>& lhs, const ConstGenericView<uint8_t>& rhs) {
    return ConstGenericView<uint8_t>::Super::op_greater_eq(lhs, rhs);
}

inline std::ostream& operator<<(std::ostream& os, const ConstGenericView<uint8_t>& v) {
    os.write((const char*) v.data(), v.size());
    return os;
}

inline ConstGenericView<uint8_t> operator "" _v(const char* str, size_t n)
{
    return ConstGenericView<uint8_t>(str, n);
}

}
