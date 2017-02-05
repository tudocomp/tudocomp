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

namespace tdc {

template<class T>
class GenericView;

/// A const view into a slice of memory.
///
/// This is an abstraction around a pointer to `T` and a size,
/// and represents N elements of memory starting at that pointer.
///
/// Creating/Copying/Modifying a View will not copy any of the data it points at.
/// Its API mostly mirrors that of a `std::vector<T>`.
template<class T>
class ConstGenericView: GenericViewBase<T, const T*> {
    using Super = GenericViewBase<T, const T*>;
    inline ConstGenericView(const Super& other): Super::GenericViewBase(other) {}
    friend class GenericView<T>;
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
    inline ConstGenericView(const T* data, size_t len): Super::GenericViewBase(data, len) {}

    /// Construct a View as a copy of `other`
    inline ConstGenericView(const ConstGenericView& other):
        Super::GenericViewBase(other) {}

    /// Construct a View as a copy of `other`
    inline ConstGenericView(const GenericView<T>& other):
        ConstGenericView(other.data(), other.size()) {}

    /// Construct a View pointing at the contents of a vector
    inline ConstGenericView(const std::vector<T>& other):
        Super::GenericViewBase(other.data(), other.size()) {}

    /// Construct a vector with the contents of this View
    using Super::operator std::vector<T>;

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
    inline bool starts_with(const T& other) const {
        return Super::starts_with(other);
    }
    inline bool starts_with(const ConstGenericView<T> other) const {
        return Super::starts_with(other);
    }
    inline bool ends_with(const T& other) const {
        return Super::ends_with(other);
    }
    inline bool ends_with(const ConstGenericView<T> other) const {
        return Super::ends_with(other);
    }

    template<class U>
    friend bool operator==(const ConstGenericView<U>& lhs, const ConstGenericView<U>& rhs);
    template<class U>
    friend bool operator!=(const ConstGenericView<U>& lhs, const ConstGenericView<U>& rhs);
    template<class U>
    friend bool operator<(const ConstGenericView<U>& lhs, const ConstGenericView<U>& rhs);
    template<class U>
    friend bool operator<=(const ConstGenericView<U>& lhs, const ConstGenericView<U>& rhs);
    template<class U>
    friend bool operator>(const ConstGenericView<U>& lhs, const ConstGenericView<U>& rhs);
    template<class U>
    friend bool operator>=(const ConstGenericView<U>& lhs, const ConstGenericView<U>& rhs);
    template<class U>
    friend void swap(ConstGenericView<U>& lhs, ConstGenericView<U>& rhs);
};

template<class T>
bool operator==(const ConstGenericView<T>& lhs, const ConstGenericView<T>& rhs) {
    return ConstGenericView<T>::Super::op_eq(lhs, rhs);
}

template<class T>
bool operator!=(const ConstGenericView<T>& lhs, const ConstGenericView<T>& rhs) {
    return ConstGenericView<T>::Super::op_not_eq(lhs, rhs);
}

template<class T>
bool operator<(const ConstGenericView<T>& lhs, const ConstGenericView<T>& rhs) {
    return ConstGenericView<T>::Super::op_less(lhs, rhs);
}

template<class T>
bool operator<=(const ConstGenericView<T>& lhs, const ConstGenericView<T>& rhs) {
    return ConstGenericView<T>::Super::op_less_eq(lhs, rhs);
}

template<class T>
bool operator>(const ConstGenericView<T>& lhs, const ConstGenericView<T>& rhs) {
    return ConstGenericView<T>::Super::op_greater(lhs, rhs);
}

template<class T>
bool operator>=(const ConstGenericView<T>& lhs, const ConstGenericView<T>& rhs) {
    return ConstGenericView<T>::Super::op_greater_eq(lhs, rhs);
}

template<class T>
void swap(ConstGenericView<T>& lhs, ConstGenericView<T>& rhs) {
    using Super = typename ConstGenericView<T>::Super;
    return swap((Super&) lhs, (Super&) rhs);
}

}

namespace std {
    template<class T>
    struct hash<tdc::ConstGenericView<T>>
    {
        size_t operator()(const tdc::ConstGenericView<T>& x) const {
            std::size_t seed;
            std::hash<T> hasher;
            for (const auto& v : x) {
                seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            return seed;
        }
    };
}
