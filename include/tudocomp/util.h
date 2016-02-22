#ifndef TUDOCOMP_UTIL_H
#define TUDOCOMP_UTIL_H

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

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
    // TODO: Maybe use nice bit ops?
    return size_t(ceil(log2(std::max(size_t(1), n) + 1)));
}

/// Returns number of bytes needed to store the amount of bits.
///
/// Example:
/// - `bytesFor(0) == 0`
/// - `bytesFor(1) == 1`
/// - `bytesFor(8) == 1`
/// - `bytesFor(9) == 2`
inline size_t bytesFor(size_t bits) {
    // TODO: Maybe use nice bit ops?
    return (size_t) ceil(bits / 8.0);
}

}

#endif
