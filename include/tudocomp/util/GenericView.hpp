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

/// A view into a slice of memory.
///
/// This is an abstraction around a pointer to `T` and a size,
/// and represents N elements of memory starting at that pointer.
///
/// Creating/Copying/Modifying a View will not copy any of the data it points at.
/// Its API mostly mirrors that of a `std::vector<T>`.
template<class T>
class GenericView: GenericViewBase<T, T*> {
    using Super = GenericViewBase<T, T*>;
    inline GenericView(const Super& other): Super::GenericViewBase(other) {}
public:
    // Type members

    using value_type             = typename Super::value_type;
    using reference              = T&;
    using const_reference        = typename Super::const_reference;
    using pointer                = T*;
    using const_pointer          = typename Super::const_pointer;
    using iterator               = pointer;
    using const_iterator         = typename Super::const_iterator;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = typename Super::const_reverse_iterator;
    using difference_type        = typename Super::difference_type;
    using size_type              = typename Super::size_type;

    // static value members

    static const size_type npos  = Super::npos;

    // Constructors

    /// Construct a empty View
    inline GenericView(): Super::GenericViewBase() {}

    /// Construct a View pointing at `len` elements starting from `data`
    inline GenericView(T* data, size_t len): Super::GenericViewBase(data, len) {}

    /// Construct a View as a copy of `other`
    inline GenericView(const GenericView& other):
        Super::GenericViewBase(other) {}

    /// Construct a View pointing at the contents of a vector
    inline GenericView(std::vector<T>& other):
        Super::GenericViewBase(other.data(), other.size()) {}

    /// Construct a vector with the contents of this View
    using Super::operator std::vector<T>;

    using Super::begin;
    inline iterator begin() {
        return this->m_data;
    }

    using Super::end;
    inline iterator end() {
        return this->m_data + this->m_size;
    }

    using Super::rbegin;
    inline reverse_iterator rbegin() {
        return std::reverse_iterator<iterator>(end());
    }

    using Super::rend;
    inline reverse_iterator rend() {
        return std::reverse_iterator<iterator>(begin());
    }

    using Super::cbegin;
    using Super::cend;
    using Super::crbegin;
    using Super::crend;

    using Super::size;
    using Super::max_size;
    using Super::empty;

    using Super::operator[];
    inline reference operator[](size_type n) {
        Super::debug_bound_check(n);
        return this->m_data[n];
    }

    using Super::at;
    inline reference at(size_type n) {
        Super::bound_check(n);
        return this->m_data[n];
    }

    using Super::front;
    inline reference front() {
        Super::debug_bound_check(0);
        return this->m_data[0];
    }

    using Super::back;
    inline reference back() {
        Super::debug_bound_check(size() - 1);
        return this->m_data[size() - 1];
    }

    using Super::data;
    inline value_type* data() noexcept {
        return this->m_data;
    }

    using Super::pop_back;
    using Super::pop_front;

    inline void swap(GenericView& other) {
        Super::swap(other);
    }

    using Super::clear;

    inline GenericView substr(size_type pos, size_type len = npos) const {
        return Super::substr(pos, len);
    }

    inline GenericView slice(size_type from, size_type to = npos) const {
        return Super::slice(from, to);
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
    friend void swap(GenericView<U>& lhs, GenericView<U>& rhs);
};

template<class T>
void swap(GenericView<T>& lhs, GenericView<T>& rhs) {
    using Super = typename GenericView<T>::Super;
    return swap((Super&) lhs, (Super&) rhs);
}

template<class T>
bool operator==(const GenericView<T>& lhs, const ConstGenericView<T>& rhs) {
    return ConstGenericView<T>(lhs) == ConstGenericView<T>(rhs);
}

template<class T>
bool operator!=(const GenericView<T>& lhs, const ConstGenericView<T>& rhs) {
    return ConstGenericView<T>(lhs) != ConstGenericView<T>(rhs);
}

template<class T>
bool operator<(const GenericView<T>& lhs, const ConstGenericView<T>& rhs) {
    return ConstGenericView<T>(lhs) < ConstGenericView<T>(rhs);
}

template<class T>
bool operator<=(const GenericView<T>& lhs, const ConstGenericView<T>& rhs) {
    return ConstGenericView<T>(lhs) <= ConstGenericView<T>(rhs);
}

template<class T>
bool operator>(const GenericView<T>& lhs, const ConstGenericView<T>& rhs) {
    return ConstGenericView<T>(lhs) > ConstGenericView<T>(rhs);
}

template<class T>
bool operator>=(const GenericView<T>& lhs, const ConstGenericView<T>& rhs) {
    return ConstGenericView<T>(lhs) >= ConstGenericView<T>(rhs);
}

template<class T>
bool operator==(const ConstGenericView<T>& lhs, const GenericView<T>& rhs) {
    return ConstGenericView<T>(lhs) == ConstGenericView<T>(rhs);
}

template<class T>
bool operator!=(const ConstGenericView<T>& lhs, const GenericView<T>& rhs) {
    return ConstGenericView<T>(lhs) != ConstGenericView<T>(rhs);
}

template<class T>
bool operator<(const ConstGenericView<T>& lhs, const GenericView<T>& rhs) {
    return ConstGenericView<T>(lhs) < ConstGenericView<T>(rhs);
}

template<class T>
bool operator<=(const ConstGenericView<T>& lhs, const GenericView<T>& rhs) {
    return ConstGenericView<T>(lhs) <= ConstGenericView<T>(rhs);
}

template<class T>
bool operator>(const ConstGenericView<T>& lhs, const GenericView<T>& rhs) {
    return ConstGenericView<T>(lhs) > ConstGenericView<T>(rhs);
}

template<class T>
bool operator>=(const ConstGenericView<T>& lhs, const GenericView<T>& rhs) {
    return ConstGenericView<T>(lhs) >= ConstGenericView<T>(rhs);
}

template<class T>
bool operator==(const GenericView<T>& lhs, const GenericView<T>& rhs) {
    return ConstGenericView<T>(lhs) == ConstGenericView<T>(rhs);
}

template<class T>
bool operator!=(const GenericView<T>& lhs, const GenericView<T>& rhs) {
    return ConstGenericView<T>(lhs) != ConstGenericView<T>(rhs);
}

template<class T>
bool operator<(const GenericView<T>& lhs, const GenericView<T>& rhs) {
    return ConstGenericView<T>(lhs) < ConstGenericView<T>(rhs);
}

template<class T>
bool operator<=(const GenericView<T>& lhs, const GenericView<T>& rhs) {
    return ConstGenericView<T>(lhs) <= ConstGenericView<T>(rhs);
}

template<class T>
bool operator>(const GenericView<T>& lhs, const GenericView<T>& rhs) {
    return ConstGenericView<T>(lhs) > ConstGenericView<T>(rhs);
}

template<class T>
bool operator>=(const GenericView<T>& lhs, const GenericView<T>& rhs) {
    return ConstGenericView<T>(lhs) >= ConstGenericView<T>(rhs);
}

}

namespace std {
    template<class T>
    struct hash<tdc::GenericView<T>> {
        size_t operator()(const tdc::GenericView<T>& x) const {
            return hash<tdc::ConstGenericView<T>>()(x);
        }
    };
}
