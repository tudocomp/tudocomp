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
class View {
    const uint8_t* m_data;
    size_t   m_size;

    inline void bound_check(size_t pos) const {
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

    using value_type             = uint8_t;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using const_reference        = const value_type&;
    using const_pointer          = const value_type*;
    using const_iterator         = const_pointer;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // static value members

    static const size_type npos = -1;

    // Constructors

    inline View(const uint8_t* data, size_t len): m_data(data), m_size(len) {}

    inline View(const View& other):   View(other.m_data, other.m_size) {}
    inline View(View&& other):        View(other.m_data, other.m_size) {}

    inline View(const std::vector<uint8_t>& other):
        View(other.data(), other.size()) {}
    inline View(const std::string& other):
        View((const uint8_t*) other.data(), other.size()) {}
    inline View(const char* other):
        View((const uint8_t*) other, strlen(other)) {}

    // Element access

    inline const_reference at(size_type pos) const {
        bound_check(pos);
        return m_data[pos];
    }

    inline const_reference operator[](size_type pos) const {
#ifdef DEBUG
        bound_check(pos);
#endif
        return m_data[pos];
    }

    inline const_reference front() const {
        return (*this)[0];
    }

    inline const_reference back() const {
        return (*this)[m_size - 1];
    }

    inline const uint8_t* data() const {
        return &front();
    }

    // Iterators

    inline const_iterator begin() const {
        return &(*this)[0];
    }
    inline const_iterator cbegin() const {
        return begin();
    }

    inline const_iterator end() const {
        return &(*this)[m_size];
    }
    inline const_iterator cend() const {
        return end();
    }

    inline const_reverse_iterator rbegin() const {
        return std::reverse_iterator<const_iterator>(end());
    }
    inline const_reverse_iterator crbegin() const {
        return rbegin();
    }

    inline const_reverse_iterator rend() const {
        return std::reverse_iterator<const_iterator>(begin());
    }
    inline const_reverse_iterator crend() const {
        return rend();
    }

    // Capacity

    inline bool empty() const {
        return m_size == 0;
    }

    inline size_type size() const {
        return m_size;
    }

    // Modifiers

    inline void swap(View& other) {
        using std::swap;

        swap(m_data, other.m_data);
        swap(m_size, other.m_size);
    }

    inline friend void swap(View& a, View& b) {
        a.swap(b);
    }

    // Slicing

    inline View substr(size_type from, size_type to = npos) const {
        // TODO
        // DCHECK(from <= to);

        if (to == npos) {
            to = m_size;
        }
        return View(&(*this)[from], to - from);
    }
};

inline bool operator==(const View& lhs, const View& rhs) {
    return (lhs.size() == rhs.size())
        && std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin());
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

}

#endif
