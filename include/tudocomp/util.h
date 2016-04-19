#ifndef TUDOCOMP_UTIL_H
#define TUDOCOMP_UTIL_H

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

#include <sdsl/bits.hpp>

namespace tudocomp {

/// Convert a vector-like type into a string showing the element values.
///
/// Useful for logging output.
///
/// Example: [1, 2, 3] -> "[1, 2, 3]"
template<class T>
std::string vec_to_debug_string(const T& s) {
    std::stringstream ss;
    ss << "[";
    if (s.size() > 0) {
        for (size_t i = 0; i < s.size() - 1; i++) {
            // crazy cast needed to bring negative char values
            // into their representation as a unsigned one
            ss << uint((unsigned char) s[i]) << ", ";
        }
        ss << uint((unsigned char) s[s.size() - 1]);
    }
    ss << "]";
    return ss.str();
}

inline std::string byte_to_nice_ascii_char(uint64_t byte) {
    std::stringstream out;
    if (byte >= 32 && byte <= 127) {
        out << "'" << char(byte) << "'";
    } else {
        out << byte;
    }
    return out.str();
}

/// Convert a vector-like type into a string by interpreting printable ASCII
/// bytes as chars, and substituting others.
///
/// Useful for logging output.
///
/// Example: [97, 32, 97, 0] -> "a a?"
template<class T>
std::string vec_as_lossy_string(const T& s, size_t start = 0,
                                char replacement = '?') {
    std::stringstream ss;
    for (size_t i = start; i < s.size(); i++) {
        if (s[i] < 32 || s[i] > 127) {
            ss << replacement;
        } else {
            ss << char(s[i]);
        }
    }
    return ss.str();
}

inline bool parse_number_until_other(std::istream& inp, char& last, size_t& out) {
    size_t n = 0;
    char c;
    bool more = true;
    while ((more = bool(inp.get(c)))) {
        if (c >= '0' && c <= '9') {
            n *= 10;
            n += (c - '0');
        } else {
            last = c;
            break;
        }
    }
    out = n;
    return more;
}

/// Returns number of bits needed to store the integer value n
///
/// The returned value will always be greater than 0
///
/// Example:
/// - `bitsFor(0b0) == 1`
/// - `bitsFor(0b1) == 1`
/// - `bitsFor(0b10) == 2`
/// - `bitsFor(0b11) == 2`
/// - `bitsFor(0b100) == 3`
inline size_t bitsFor(size_t n) {
    if(n == 0) {
        return 1U;
    } else {
        return sdsl::bits::hi(n) + 1;
    }
}

/// integer division with rounding up
inline size_t idiv_ceil(size_t a, size_t b) {
    return (a / b) + ((a % b) > 0);
}

/// Returns number of bytes needed to store the amount of bits.
///
/// Example:
/// - `bytesFor(0) == 0`
/// - `bytesFor(1) == 1`
/// - `bytesFor(8) == 1`
/// - `bytesFor(9) == 2`
inline size_t bytesFor(size_t bits) {
    return idiv_ceil(bits, 8U);
}

}

// this codebase is using c++11 but would really like to use this function...
namespace std {
    template<class T> struct _Unique_if {
        typedef unique_ptr<T> _Single_object;
    };

    template<class T> struct _Unique_if<T[]> {
        typedef unique_ptr<T[]> _Unknown_bound;
    };

    template<class T, size_t N> struct _Unique_if<T[N]> {
        typedef void _Known_bound;
    };

    template<class T, class... Args>
        typename _Unique_if<T>::_Single_object
        make_unique(Args&&... args) {
            return unique_ptr<T>(new T(std::forward<Args>(args)...));
        }

    template<class T>
        typename _Unique_if<T>::_Unknown_bound
        make_unique(size_t n) {
            typedef typename remove_extent<T>::type U;
            return unique_ptr<T>(new U[n]());
        }

    template<class T, class... Args>
        typename _Unique_if<T>::_Known_bound
        make_unique(Args&&...) = delete;
}

inline std::string indent_lines(std::string sentence, size_t ident) {
    std::stringstream ss(sentence);
    std::string to;
    std::stringstream ret;

    bool first = true;
    while(std::getline(ss,to,'\n')){
        if (!first) {
            ret << "\n";
        }
        ret << std::setw(ident) << "" << to;
        first = false;
    }

    return ret.str();
}

inline std::string first_line(std::string sentence) {
    std::stringstream ss(sentence);
    std::string to;

    if(std::getline(ss,to,'\n')){
        return to;
    } else {
        return "";
    }
}

#endif
