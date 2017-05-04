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
/// This is an abstraction around a const pointer to `T` and a size,
/// and represents N elements consecutive in memory.
///
/// Creating/Copying/Modifying a View will not copy any of the data it points at.
/// Its API mostly mirrors that of a `std::vector<T>`.
template<class T>
class ConstGenericView: GenericViewBase<T, const T*> {
    using Super = GenericViewBase<T, const T*>;
    inline ConstGenericView(const Super& other): Super::GenericViewBase(other) {}
    friend class GenericView<T>;

    using Super::op_eq;
    using Super::op_not_eq;
    using Super::op_greater;
    using Super::op_greater_eq;
    using Super::op_less;
    using Super::op_less_eq;
public:
    using value_type             = typename Super::value_type;
    using const_reference        = typename Super::const_reference;
    using const_pointer          = typename Super::const_pointer;
    using const_iterator         = typename Super::const_iterator;
    using const_reverse_iterator = typename Super::const_reverse_iterator;
    using difference_type        = typename Super::difference_type;
    using size_type              = typename Super::size_type;

    /// Sentinel value indicating a index at the end of the view.
    static const size_type npos  = Super::npos;

    /// Construct a empty View
    inline ConstGenericView():
        Super::GenericViewBase() {}

    /// Construct a View pointing at `len` elements starting from `data`
    inline ConstGenericView(const T* data, size_t len):
        Super::GenericViewBase(data, len) {}

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
    inline operator std::vector<T>() const {
        return Super::operator std::vector<T>();
    }

    /// Begin of iterator
    inline const_iterator begin() const {
        return Super::begin();
    }

    /// End of iterator
    inline const_iterator end() const {
        return Super::end();
    }

    /// Begin of reverse iterator
    inline const_reverse_iterator rbegin() const {
        return Super::rbegin();
    }

    /// End of reverse iterator
    inline const_reverse_iterator rend() const {
        return Super::rend();
    }

    /// Begin of const iterator
    inline const_iterator cbegin() const {
        return Super::cbegin();
    }

    /// End of const iterator
    inline const_iterator cend() const {
        return Super::cend();
    }

    /// Begin of const reverse iterator
    inline const_reverse_iterator crbegin() const {
        return Super::crbegin();
    }

    /// End of const reverse iterator
    inline const_reverse_iterator crend() const {
        return Super::crend();
    }

    /// Returns size of the View
    inline size_type size() const {
        return Super::size();
    }

    /// Returns max size of the View. Always the same as `size()`
    inline size_type max_size() const {
        return Super::max_size();
    }

    /// Returns `true` if empty
    inline bool empty() const {
        return Super::empty();
    }

    /// Access the element at `pos`
    ///
    /// This method is bounds checked in debug builds
    inline const_reference operator[](size_type pos) const {
        return Super::operator[](pos);
    }

    /// Access the element at `pos`
    ///
    /// This method is always bounds checked
    inline const_reference at(size_type pos) const {
        return Super::at(pos);
    }

    /// Access the first element
    inline const_reference front() const {
        return Super::front();
    }

    /// Access the last element
    inline const_reference back() const {
        return Super::back();
    }

    /// The backing memory location
    inline const value_type* data() const noexcept {
        return Super::data();
    }

    /// Remove the last element from the View.
    inline void pop_back() {
        Super::pop_back();
    }

    /// Remove the first element from the View.
    inline void pop_front() {
        Super::pop_front();
    }

    /// Swap two Views
    inline void swap(ConstGenericView& other) {
        Super::swap(other);
    }

    /// Sets the size to 0
    inline void clear() {
        Super::clear();
    }

    /// Construct a new View that is a sub view into the current one.
    ///
    /// The returned view will be a "position-length" slice,
    /// and cover the bytes starting at `pos` and ending at `pos + len`.
    ///
    /// Passing `npos` to `len` will create a slice until the end of the View
    ///
    /// This method covers the same basic operation as `slice()`, but
    /// mirrors the semantic of `std::string::substr`.
    ///
    /// # Example
    ///
    /// `View("abcd").substr(1, 2) == View("bc")`
    inline ConstGenericView substr(size_type pos, size_type len = npos) const {
        return ConstGenericView(Super::substr(pos, len));
    }

    /// Construct a new View that is a sub view into the current one.
    ///
    /// The returned view will be a "from-to" slice,
    /// and cover the bytes starting at `from` and ending *just before* `to`.
	/// T.slice(a,b) gives T[a..b-1]
    ///
    /// Passing `npos` to `to` will create a slice until the end of the View
    ///
    /// # Example
    ///
    /// `View("abcd").slice(1, 3) == View("bc")`
    inline ConstGenericView slice(size_type from, size_type to = npos) const {
        return ConstGenericView(Super::slice(from, to));
    }

    /// Removes the first `n` elements from the View
    inline void remove_prefix(size_type n) {
        return Super::remove_prefix(n);
    }

    /// Removes the last `n` elements from the View
    inline void remove_suffix(size_type n) {
        return Super::remove_suffix(n);
    }

    /// Returns `true` if the View starts with the literal `other`
    inline bool starts_with(const T& other) const {
        return Super::starts_with(other);
    }

    /// Returns `true` if the View starts with the sequence of literals
    /// contained in `other`.
    inline bool starts_with(const ConstGenericView<T>& other) const {
        return Super::starts_with(other);
    }

    /// Returns `true` if the View ends with the literal `other`
    inline bool ends_with(const T& other) const {
        return Super::ends_with(other);
    }

    /// Returns `true` if the View ends with the sequence of literals
    /// contained in `other`.
    inline bool ends_with(const ConstGenericView<T>& other) const {
        return Super::ends_with(other);
    }

    template<class U>
    friend void swap(ConstGenericView<U>& lhs, ConstGenericView<U>& rhs);

    template<typename U>
    friend bool operator==(const ConstGenericView<U>& lhs,
                           const ConstGenericView<U>& rhs);
    template<typename U>
    friend bool operator!=(const ConstGenericView<U>& lhs,
                           const ConstGenericView<U>& rhs);
    template<typename U>
    friend bool operator<(const ConstGenericView<U>& lhs,
                          const ConstGenericView<U>& rhs);
    template<typename U>
    friend bool operator<=(const ConstGenericView<U>& lhs,
                           const ConstGenericView<U>& rhs);
    template<typename U>
    friend bool operator>(const ConstGenericView<U>& lhs,
                          const ConstGenericView<U>& rhs);
    template<typename U>
    friend bool operator>=(const ConstGenericView<U>& lhs,
                           const ConstGenericView<U>& rhs);
};

template<class T>
bool operator==(const ConstGenericView<T>& lhs, const ConstGenericView<T>& rhs) {
    return ConstGenericView<T>::op_eq(lhs, rhs);
}

template<class T>
bool operator!=(const ConstGenericView<T>& lhs, const ConstGenericView<T>& rhs) {
    return ConstGenericView<T>::op_not_eq(lhs, rhs);
}

template<class T>
bool operator<(const ConstGenericView<T>& lhs, const ConstGenericView<T>& rhs) {
    return ConstGenericView<T>::op_less(lhs, rhs);
}

template<class T>
bool operator<=(const ConstGenericView<T>& lhs, const ConstGenericView<T>& rhs) {
    return ConstGenericView<T>::op_less_eq(lhs, rhs);
}

template<class T>
bool operator>(const ConstGenericView<T>& lhs, const ConstGenericView<T>& rhs) {
    return ConstGenericView<T>::op_greater(lhs, rhs);
}

template<class T>
bool operator>=(const ConstGenericView<T>& lhs, const ConstGenericView<T>& rhs) {
    return ConstGenericView<T>::op_greater_eq(lhs, rhs);
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
