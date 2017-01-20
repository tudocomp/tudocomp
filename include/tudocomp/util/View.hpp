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
#include <tudocomp/def.hpp>

namespace tdc {

/// A view into a slice of memory.
///
/// This is an abstraction around a `const uint8_t*` pointer and a `size_t` size,
/// and represents N bytes of memory starting at that pointer.
///
/// Creating/Copying/Modifying a View will not copy any of the data it points at.
class View {
    const uliteral_t* m_data;
    size_t   m_size;

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

public:
    // Type members

    using value_type             = uliteral_t;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using const_reference        = const value_type&;
    using const_pointer          = const value_type*;
    using const_iterator         = const_pointer;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // static value members

    static const size_type npos = -1;

    // Constructors

    /// Construct a empty View
    inline View(): View("") {}

    /// Construct a View pointing at `len` elements starting from `data`
    inline View(const uint8_t* data, size_t len): m_data(data), m_size(len) {}

    /// Construct a View as a copy of `other`
    inline View(const View& other):   View(other.m_data, other.m_size) {}

    /// Construct a View pointing at the contents of a vector
    inline View(const std::vector<uint8_t>& other):
        View(other.data(), other.size()) {}

    /// Construct a View pointing at the contents of a string
    inline View(const std::string& other):
        View((const uint8_t*) other.data(), other.size()) {}

    /// Construct a View equal to the (offset)-th suffix of other
    inline View(const View& other, size_t offset):
        View((const uint8_t*) other.data()+offset, other.size()-offset) { DCHECK_LE(offset, other.size()); }

    /// Construct a View pointing at a C-style null terminated string.
    ///
    /// Can be used to construct from a string literal
    inline View(const char* other):
        View((const uint8_t*) other, strlen(other)) {}

    /// Construct a View pointing at `len` elements starting from `data`
    inline View(const char* data, size_t len):
        View((const uint8_t*) data, len) {}

    // Conversion

    /// Construct a string with the contents of this View
    inline operator std::string() const {
        return std::string(cbegin(), cend());
    }

    /// Construct a vector with the contents of this View
    inline operator std::vector<uint8_t>() const {
        return std::vector<uint8_t>(cbegin(), cend());
    }

    // Element access

    /// Access the element at `pos`
    ///
    /// This method is always bounds checked
    inline const_reference at(size_type pos) const {
        bound_check(pos);
        return m_data[pos];
    }

    /// Access the element at `pos`
    ///
    /// This method is bounds checked in debug builds
    inline const_reference operator[](size_type pos) const {
        debug_bound_check(pos);
        return m_data[pos];
    }

    /// Access the first element
    inline const_reference front() const {
        return (*this)[0];
    }

    /// Access the last element
    inline const_reference back() const {
        return (*this)[m_size - 1];
    }

    /// The backing memory location
    inline const uint8_t* data() const {
        return m_data;
    }

    // Iterators

    /// Begin of iterator
    inline const_iterator begin() const {
        return m_data;
    }
    /// Begin of const iterator
    inline const_iterator cbegin() const {
        return begin();
    }

    /// End of iterator
    inline const_iterator end() const {
        return m_data + m_size;
    }
    /// End of const iterator
    inline const_iterator cend() const {
        return end();
    }

    /// Begin of reverse iterator
    inline const_reverse_iterator rbegin() const {
        return std::reverse_iterator<const_iterator>(end());
    }
    /// Begin of const reverse iterator
    inline const_reverse_iterator crbegin() const {
        return rbegin();
    }

    /// End of reverse iterator
    inline const_reverse_iterator rend() const {
        return std::reverse_iterator<const_iterator>(begin());
    }
    /// End of const reverse iterator
    inline const_reverse_iterator crend() const {
        return rend();
    }

    // Capacity

    /// Returns `true` if empty
    inline bool empty() const {
        return m_size == 0;
    }

    /// Returns size of the View
    inline size_type size() const {
        return m_size;
    }

    /// Returns max size of the View. Always the same as `size()`
    inline size_type max_size() const {
        return size();
    }

    // Slicing

    /// Construct a new View that is a sub view into the current one.
    ///
    /// The returned view will be a "from-to" slice,
    /// and cover the bytes starting at `from` and ending at `to`.
    ///
    /// Passing `npos` to `to` will create a slice until the end of the View
    ///
    /// # Example
    ///
    /// `View("abcd").slice(1, 3) == View("bc")`
    inline View slice(size_type from, size_type to = npos) const {
        if (to == npos) {
            to = m_size;
        }

        DCHECK_LE(from, to);
        DCHECK_LE(from, size());
        DCHECK_LE(to, size());

        return View(m_data + from, to - from);
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
    inline View substr(size_type pos, size_type len = npos) const {
        if (len == npos) {
            len = m_size - pos;
        }

        return slice(pos, pos + len);
    }

    // Modifiers

    /// Swap two Views
    inline void swap(View& other) {
        using std::swap;

        swap(m_data, other.m_data);
        swap(m_size, other.m_size);
    }

    /// Swap two Views
    inline friend void swap(View& a, View& b) {
        a.swap(b);
    }

    /// Sets the size to 0
    inline void clear() {
        m_size = 0;
    }

    /// Removes the first `n` elements from the View
    inline void remove_prefix(size_type n) {
        *this = slice(n);
    }

    /// Removes the last `n` elements from the View
    inline void remove_suffix(size_type n) {
        *this = slice(0, m_size - n);
    }

    // string predicates

    /// Returns `true` if the View starts with `c`
    inline bool starts_with(uint8_t c) const {
        return !empty() && (front() == c);
    }
    /// Returns `true` if the View starts with `c`
    inline bool starts_with(char c) const {
        return starts_with(uint8_t(c));
    }
    /// Returns `true` if the View starts with `x`
    inline bool starts_with(const View& x) const;
    /// Returns `true` if the View ends with `c`
    inline bool ends_with(uint8_t c) const {
        return !empty() && (back() == c);
    }
    /// Returns `true` if the View ends with `c`
    inline bool ends_with(char c) const {
        return ends_with(uint8_t(c));
    }
    /// Returns `true` if the View ends with `x`
    inline bool ends_with(const View& x) const;

};

inline bool operator==(const View& lhs, const View& rhs) {
    // TODO: memcmp!
    return (lhs.size() == rhs.size())
        && (std::memcmp(lhs.data(), rhs.data(), lhs.size()) == 0);
}
inline bool operator!=(const View& lhs, const View& rhs) {
    return !(lhs == rhs);
}
inline bool operator<(const View& lhs, const View& rhs) {
    return std::lexicographical_compare(lhs.cbegin(), lhs.cend(),
                                        rhs.cbegin(), rhs.cend());
}
inline bool operator>(const View& lhs, const View& rhs) {
    return std::lexicographical_compare(lhs.cbegin(), lhs.cend(),
                                        rhs.cbegin(), rhs.cend(),
                                        [](const uint8_t& l, const uint8_t& r){
                                            return l > r;
                                        });
}
inline bool operator<=(const View& lhs, const View& rhs) {
    return !(lhs > rhs);
}
inline bool operator>=(const View& lhs, const View& rhs) {
    return !(lhs < rhs);
}

inline std::ostream& operator<<(std::ostream& os, const View& v) {
    os.write((const char*) v.data(), v.size());
    return os;
}

inline bool View::starts_with(const View& x) const {
    return (x.size() <= size())
        && (slice(0, x.size()) == x);
}

inline bool View::ends_with(const View& x) const {
    return (x.size() <= size())
        && (slice(size() - x.size()) == x);
}

inline View operator "" _v(const char* str, size_t n)
{
    return View(str, n);
}

using string_ref = View;

}

