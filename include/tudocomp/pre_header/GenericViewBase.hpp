#ifndef TUDOCOMP_GENERIC_VIEW_BASE_H
#define TUDOCOMP_GENERIC_VIEW_BASE_H

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

namespace tdc {

/// \cond INTERNAL

template<class T, class P>
class GenericViewBase {
    // TODO: Hack, fix sometime
    //protected:
public:

    friend class GenericViewBase<T, const T*>;
    friend class GenericViewBase<T, T*>;

    using value_type             = T;
    using const_reference        = const T&;
    using const_pointer          = const T*;
    using const_iterator         = const_pointer;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using difference_type        = ptrdiff_t;
    using size_type              = size_t;

    static const size_type npos = -1;

    P      m_data;
    size_t m_size;

    inline void bound_check(size_t pos) const {
        if (pos >= m_size) {
            std::stringstream ss;
            ss << "accessing view with bounds [0, ";
            ss << m_size;
            ss << ") at out-of-bounds index ";
            ss << pos;
            throw std::out_of_range(ss.str());
        }
    }

    inline void debug_bound_check(size_t pos) const {
#ifdef DEBUG
        bound_check(pos);
#endif
    }

    inline GenericViewBase(P data, size_t size): m_data(data), m_size(size) {}
    inline GenericViewBase(): GenericViewBase(P(""), 0) {} // Need a valid static pointer...
    inline GenericViewBase(const GenericViewBase& other):
        GenericViewBase(other.m_data, other.m_size) {}
    inline GenericViewBase(const std::vector<T>& other):
        GenericViewBase(other.data(), other.size()) {}
    template<size_t N>
    inline GenericViewBase(const std::array<T, N>& other):
        GenericViewBase(other.data(), other.size()) {}

    inline operator std::vector<T>() const {
        return std::vector<T>(cbegin(), cend());
    }

    inline GenericViewBase& operator=(const GenericViewBase& other) {
        m_data = other.m_data;
        m_size = other.m_size;
        return *this;
    }

    inline const_iterator begin() const {
        return m_data;
    }

    inline const_iterator end() const {
        return m_data + m_size;
    }

    inline const_reverse_iterator rbegin() const {
        return std::reverse_iterator<const_iterator>(end());
    }

    inline const_reverse_iterator rend() const {
        return std::reverse_iterator<const_iterator>(begin());
    }

    inline const_iterator cbegin() const {
        return m_data;
    }

    inline const_iterator cend() const {
        return m_data + m_size;
    }

    inline const_reverse_iterator crbegin() const {
        return std::reverse_iterator<const_iterator>(cend());
    }

    inline const_reverse_iterator crend() const {
        return std::reverse_iterator<const_iterator>(cbegin());
    }

    inline size_type size() const {
        return m_size;
    }

    inline size_type max_size() const {
        return size();
    }

    inline bool empty() const {
        return size() == 0;
    }

    inline const_reference operator[](size_type n) const {
        debug_bound_check(n);
        return m_data[n];
    }

    inline const_reference at(size_type n) const {
        bound_check(n);
        return m_data[n];
    }

    inline const_reference front() const {
        debug_bound_check(0);
        return m_data[0];
    }

    inline const_reference back() const {
        debug_bound_check(size() - 1);
        return m_data[size() - 1];
    }

    inline const value_type* data() const noexcept {
        return m_data;
    }

    inline void pop_back() {
        m_size--;
    }

    inline void pop_front() {
        m_data++;
        m_size--;
    }

    inline void swap(GenericViewBase& other) {
        using std::swap;
        swap(m_data, other.m_data);
        swap(m_size, other.m_size);
    }

    inline void clear() {
        m_size = 0;
    }

    inline GenericViewBase substr(size_type from, size_type to = npos) const {
        if (to == npos) {
            to = m_size;
        }

        DCHECK(from <= to);
        DCHECK(from <= size());
        DCHECK(to <= size());

        return GenericViewBase(m_data + from, to - from);
    }

    inline void remove_prefix(size_type n) {
        *this = substr(n);
    }

    inline void remove_suffix(size_type n) {
        *this = substr(0, m_size - n);
    }

    inline bool starts_with(const T& c) const {
        return !empty() && (front() == c);
    }

    inline bool starts_with(const GenericViewBase<T, const T*>& x) const {
        GenericViewBase<T, const T*> y(m_data, m_size);
        return (x.size() <= y.size()) && op_eq(y.substr(0, x.size()), x);
    }

    inline bool ends_with(const T& c) const {
        return !empty() && (back() == c);
    }

    inline bool ends_with(const GenericViewBase<T, const T*>& x) const {
        GenericViewBase<T, const T*> y(m_data, m_size);
        return (x.size() <= y.size()) && op_eq(y.substr(size() - x.size()), x);
    }

    template<class U, class Q>
    friend void swap(GenericViewBase<U, Q>& lhs, GenericViewBase<U, Q>& rhs);


    inline static bool op_eq(const GenericViewBase<T, const T*>& lhs, const GenericViewBase<T, const T*>& rhs) {
        if (lhs.size() != rhs.size()) return false;
        return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin());
    }

    inline static bool op_not_eq(const GenericViewBase<T, const T*>& lhs, const GenericViewBase<T, const T*>& rhs) {
        return !(op_eq(lhs, rhs));
    }

    inline static bool op_less(const GenericViewBase<T, const T*>& lhs, const GenericViewBase<T, const T*>& rhs) {
        return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
    }

    inline static bool op_less_eq(const GenericViewBase<T, const T*>& lhs, const GenericViewBase<T, const T*>& rhs) {
        return !(op_greater(lhs, rhs));
    }

    inline static bool op_greater(const GenericViewBase<T, const T*>& lhs, const GenericViewBase<T, const T*>& rhs) {
        return std::lexicographical_compare(rhs.cbegin(), rhs.cend(), lhs.cbegin(), lhs.cend());
    }

    inline static bool op_greater_eq(const GenericViewBase<T, const T*>& lhs, const GenericViewBase<T, const T*>& rhs) {
        return !(op_less(lhs, rhs));
    }
};

template<class T, class Q>
void swap(GenericViewBase<T, Q>& lhs, GenericViewBase<T, Q>& rhs) {
    lhs.swap(rhs);
}

/// \endcond

}

#endif
