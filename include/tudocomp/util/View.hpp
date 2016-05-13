#ifndef TUDOCOMP_VIEW_H
#define TUDOCOMP_VIEW_H

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

namespace tudocomp {

/// A view into a slice of memory.
template<class T>
class View {
    T*     m_data;
    size_t m_size;

    void bound_check(size_t pos) {
        if (pos >= m_size) {
            std::stringstream ss;
            ss << "indexed view of size ";
            ss << m_size;
            ss << " with out-of-bounds value ";
            ss << pos;
            throw std::out_of_range(ss.str());
        }
    }

public:
    // Type members

    using value_type             = T;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using reference              = value_type&;
    using const_reference        = const value_type&;
    using pointer                = value_type*;
    using const_pointer          = const value_type*;
    using iterator               = pointer;
    using const_iterator         = const_pointer;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // static value members

    static const size_type npos = -1;

    // Constructors

    View(T* data, size_t len): m_data(data), m_size(len) {}

    View(const View<T>& other):   View(other.m_data, other.m_size) {}
    View(View<T>&& other):        View(other.m_data, other.m_size) {}

    View(std::vector<T>& other): View(other.data(), other.size()) {}

    // Element access

    reference       at(size_type pos) {
        bound_check(pos);
        return m_data[pos];
    }
    const_reference at(size_type pos) const {
        bound_check(pos);
        return m_data[pos];
    }

    reference       operator[](size_type pos) {
#ifdef DEBUG
        bound_check(pos);
#endif
        return m_data[pos];
    }
    const_reference operator[](size_type pos) const {
#ifdef DEBUG
        bound_check(pos);
#endif
        return m_data[pos];
    }

    reference front() {
        return (*this)[0];
    }
    const_reference front() const {
        return (*this)[0];
    }

    reference back() {
        return (*this)[m_size - 1];
    }
    const_reference back() const {
        return (*this)[m_size - 1];
    }

    T* data() {
        return front();
    }
    const T* data() const {
        return front();
    }

    // Iterators

    iterator begin() {
        return (*this)[0];
    }
    const_iterator begin() const {
        return (*this)[0];
    }
    const_iterator cbegin() const {
        return begin();
    }

    iterator end() {
        return (*this)[m_size];
    }
    const_iterator end() const {
        return (*this)[m_size];
    }
    const_iterator cend() const {
        return end();
    }

    reverse_iterator rbegin() {
        return std::reverse_iterator<iterator>(end());
    }
    const_reverse_iterator rbegin() const {
        return std::reverse_iterator<const_reverse_iterator>(end());
    }
    const_reverse_iterator crbegin() const {
        return rbegin();
    }

    reverse_iterator rend() {
        return std::reverse_iterator<iterator>(begin());
    }
    const_reverse_iterator rend() const {
        return std::reverse_iterator<const_reverse_iterator>(begin());
    }
    const_reverse_iterator crend() const {
        return rend();
    }

    // Capacity

    bool empty() const {
        return m_size == 0;
    }

    size_type size() const {
        return m_size;
    }

    // Modifiers

    void swap(View<T>& other) {
        using std::swap;

        swap(m_data, other.m_data);
        swap(m_size, other.m_size);
    }

    friend void swap(View<T>& a, View<T>& b) {
        a.swap(b);
    }

    // Slicing

    View slice(size_type from, size_type to = npos) {
        // TODO
        // DCHECK(from <= to);

        if (to == npos) {
            to = m_size;
        }
        return View(&(*this)[from], to - from);
    }

    View substr(size_type from, size_type to = npos) {
        return slice(from, to);
    }
};

template<class T>
bool operator==(const View<T>& lhs, const View<T>& rhs) {
    return (lhs.size() == rhs.size())
        && std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin());
}
template<class T>
bool operator!=(const View<T>& lhs, const View<T>& rhs) {
    return !(lhs == rhs);
}
template<class T>
bool operator<(const View<T>& lhs, const View<T>& rhs) {
    return std::lexicographical_compare(lhs.cbegin(), lhs.cend(),
                                        rhs.cbegin(), rhs.cend());
}
template<class T>
bool operator>(const View<T>& lhs, const View<T>& rhs) {
    struct greater {
        inline bool cmp(const T& l, const T& r) {
            return l > r;
        }
    };

    return std::lexicographical_compare(lhs.cbegin(), lhs.cend(),
                                        rhs.cbegin(), rhs.cend(),
                                        greater());
}
template<class T>
bool operator<=(const View<T>& lhs, const View<T>& rhs) {
    return !(lhs > rhs);
}
template<class T>
bool operator>=(const View<T>& lhs, const View<T>& rhs) {
    return !(lhs < rhs);
}

}

#endif
