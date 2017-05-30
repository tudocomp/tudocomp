#pragma once

#include <memory>
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

#include <tudocomp/util/View.hpp>

namespace tdc {

/// \brief Builds the string representation of a vector of byte values,
/// sorrounded by square brackets (\c \[ and \c \]).
///
/// Example: [1, 2, 3] yields \c "[1, 2, 3]".
///
/// \tparam T The byte vector type.
/// \param s The byte vector.
/// \return The string representation of the byte vector.
template<class T>
std::string vec_to_debug_string(const T& s, size_t indent = 0) {
    std::stringstream ss;
    ss << "[";
    if (s.size() > 0) {
        for (size_t i = 0; i < s.size() - 1; i++) {
            // crazy cast needed to bring negative char values
            // into their representation as a unsigned one
            ss << std::setw(indent) << uint((unsigned char) s[i]) << ", ";
        }
        ss << std::setw(indent) << uint((unsigned char) s[s.size() - 1]);
    }
    ss << "]";
    return ss.str();
}

/// \brief Builds the string representation of an array of printable values,
/// sorrounded by square brackets (\c \[ and \c \]).
///
/// Example: [1, 2, 3] yields \c "[1, 2, 3]".
///
/// \tparam T The byte array type.
/// \param s The byte array.
/// \return The string representation of the byte array.
template<class T>
std::string arr_to_debug_string(const T* s, size_t length) {
    if(length == 0) return "[]";
    std::stringstream ss;
    ss << "[";
	for (size_t i = 0; i < length - 1; ++i) {
		ss << s[i] << ", ";
	}
	ss << s[length - 1];
    ss << "]";
    return ss.str();
}

/// \brief Converts a byte value into its ASCII representation sorrounded by
/// single quotes (\c ') or its string representation.
///
/// \param byte The byte value.
/// \return \c 'char(byte)' if the byte represents an ASCII symbol,
///         the string representation of the byte value otherwise.
inline std::string byte_to_nice_ascii_char(uint64_t byte) {
    std::stringstream out;
    if (byte >= 32 && byte <= 127) {
        out << "'" << char(byte) << "'";
    } else {
        out << byte;
    }
    return out.str();
}

/// \brief Converts a vector of bytes into a readable ASCII string,
/// substituting non-ASCII symbols.
///
/// Example: [97, 32, 97, 0] is converted to \c "a a?".
///
/// \tparam T The input vector type.
/// \param s The input vector.
/// \param start The indext at which to start processing the vector.
/// \param replacement The character to replace non-ASCII symbols with.
/// \return The ASCII string representation of the byte vector.
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

/// \brief Represent a value as a string.
///
/// This is done by creating a \c std::stringstream and writing the value
/// into it using the \c << operator.
///
/// \tparam T the value type
/// \param v the value to convert
/// \return the string representation of the given value
template<typename T>
inline std::string to_string(const T& v) {
    std::stringstream ss;
    ss << v;
    return ss.str();
}

/// \brief Parses the given string into a value.
///
/// This is done by creating a \c std::stringstream that writes to a value
/// of the desired type using the \c >> operator.
///
/// \tparam T the value type
/// \param s the string to parse
/// \return the parsed value
template<typename T> T lexical_cast(const std::string& s) {
    T val;
    std::stringstream(s) >> val;
    return val;
}

template<> std::string lexical_cast(const std::string& s) {
    return std::string(s);
}

/// \brief Tests if the given string contains an expression that can be
///        interpreted as a boolean \c true value.
///
/// This is the case if the string is one of \c "true", \c "1", \c "yes"
/// or \c "on" (case-insensitive).
///
/// \param str the string to test
/// \return \c true if the above description matches, \c false otherwise
bool is_true(const std::string& str) {
    std::string s(str);
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return (s == "true" || s == "1" || s == "yes" || s == "on");
}

inline void debug_print_uint64_t(uint64_t v) {
    for(int i = 0; i < 64; i++) {
        if (i > 0 && i % 8 == 0) {
            std::cout << " ";
        }
        std::cout << int((v >> (63 - i)) & 1);
    }
    std::cout << "\n";
}

/// \brief Reads digits from the input stream (\c 0 to \c 9) until a non-digit
/// character is reached and parses them as an integer.
///
/// If the initial stream position contains a non-digit, a value of zero is
/// parsed.
///
/// \param inp The input stream.
/// \param last Used to store the first non-digit character read upon
///             termination.
/// \param out Used to store the parsed integer value. This will be zero if no
///            digit character is found.
/// \return \c true if there are more characters left on the stream after
///         reading to the first non-digit, \c false if the stream is over.
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

/// \brief Computes the highest set bit in an integer variable
inline constexpr uint_fast8_t bits_hi(uint64_t x) {
	return x == 0 ? 0 : 64 - __builtin_clzll(x);
}

/// \brief Computes the number of bits required to store the given integer
/// value.
///
/// This is equivalent to the binary logarithm rounded up to the next integer.
///
/// Examples:
/// - `bits_for(0b0) == 1`
/// - `bits_for(0b1) == 1`
/// - `bits_for(0b10) == 2`
/// - `bits_for(0b11) == 2`
/// - `bits_for(0b100) == 3`
///
/// \param n The integer to be stored.
/// \return The amount of bits required to store the value (guaranteed to be
/// greater than zero).
inline constexpr uint_fast8_t bits_for(size_t n) {
    return n == 0 ? 1U : bits_hi(n);
}

/// \brief Performs an integer division with the result rounded up to the
/// next integer.
///
/// \param a The dividend.
/// \param b The divisor.
/// \return The quotient, rounded up to the next integer value.
inline constexpr size_t idiv_ceil(size_t a, size_t b) {
    return (a / b) + ((a % b) > 0);
}

/// \brief Computes the number of bytes needed to store the given integer
/// value.
///
/// This is equivalent to binary logarithm divided by 8 and rounded up to the
/// next integer.
///
/// Examples:
/// - `bytes_for(0) == 1`
/// - `bytes_for(255) == 1`
/// - `bytes_for(256) == 2`
/// - `bytes_for(65535) == 2`
/// - `bytes_for(65536) == 3`
/// - etc.
///
/// \param n The integer to be stored.
/// \return The amount of bits required to store the value (guaranteed to be
/// greater than zero).
inline constexpr uint_fast8_t bytes_for(size_t n) {
    return idiv_ceil(bits_for(n), 8U);
}

/// \brief Creates the cross product of a set of elements given a product
/// function.
///
/// The function \c f is applied to each possible pair of elements in the
/// input set and the results are stored into the result vector.
///
/// \tparam The element type.
/// \param vs The input set.
/// \param f The product function.
/// \return The results of the product function for each pair.
template<class T>
std::vector<T> cross(std::vector<std::vector<T>>&& vs,
                     std::function<T(T, T&)> f) {
    auto remaining = vs;
    if (remaining.size() == 0) {
        return {};
    }

    std::vector<T> first = std::move(remaining[0]);
    remaining.erase(remaining.begin());

    auto next = cross(std::move(remaining), f);

    if (next.size() == 0) {
        return first;
    } else {
        std::vector<T> r;
        for (auto& x : first) {
            for (auto& y : next) {
                r.push_back(f(x, y));
            }
        }
        return r;
    }
}

/// \brief Splits the input string into lines (separated by \c \\n).
///
/// \param s The input string.
/// \return A vector containing the individual lines.
inline std::vector<std::string> split_lines(const std::string& s) {
    std::stringstream ss(s);
    std::string to;
    std::vector<std::string> ret;

    while(std::getline(ss,to,'\n')) {
        ret.push_back(to);
    }

    return ret;
}

/// \brief Indents each line of a string (separated by \c \\n) by the
/// specified amount of spaces.
///
/// \param s The input string.
/// \param indent The amount of spaces to indent each line.
/// \return The indented string.
inline std::string indent_lines(const std::string& s, size_t indent) {
    std::stringstream ss(s);
    std::string to;
    std::stringstream ret;

    bool first = true;
    while(std::getline(ss,to,'\n')){
        if (!first) {
            ret << "\n";
        }
        ret << std::setw(indent) << "" << to;
        first = false;
    }

    return ret.str();
}

/// \brief Renders the given dataset into an ASCII table.
///
/// \param data The data vector. Each contained string represents a cell in
///             the table. Every \c cols entries make up one row.
/// \param cols The amount of columns to display the data in.
/// \param draw_grid If \c true, draws an ASCII grid for the cells.
/// \return The rendered ASCII table string.
inline std::string make_table(const std::vector<std::string>& data,
                              size_t cols,
                              bool draw_grid = true) {
    std::vector<size_t> widths;
    std::vector<size_t> heights;
    std::vector<std::vector<std::string>> data_lines;

    // Gather inidividual cell dimensions
    for (auto& elem : data) {
        size_t w = 0;
        size_t h = 0;

        data_lines.push_back(split_lines(elem));
        auto& lines = data_lines[data_lines.size() - 1];

        for (auto& line : lines) {
            w = std::max(w, line.size());
        }

        h = lines.size();


        widths.push_back(w);
        heights.push_back(h);
    }

    std::vector<size_t> col_widths(cols, 0);
    std::vector<size_t> row_heights(data.size() / cols, 0);

    // Calculate max. common row/column dimensions
    for (size_t i = 0; i < data.size(); i++) {
        size_t x = i % cols;
        size_t y = i / cols;

        col_widths[x] = std::max(col_widths[x], widths[i]);
        row_heights[y] = std::max(row_heights[y], heights[i]);
    }

    // Adjust lines to match
    for (size_t i = 0; i < data.size(); i++) {
        size_t x = i % cols;
        size_t y = i / cols;

        while (data_lines[i].size() < row_heights[y]) {
            data_lines[i].push_back("");
        }

        for (auto& line : data_lines[i]) {
            size_t pad = col_widths[x] - line.size();
            line.append(pad, ' ');
        }
    }

    std::stringstream ret;

    // Create formatted table

    const char ROW = '-';
    const std::string COL = " | ";

    size_t total_width = 1;
    for (size_t i = 0; i < col_widths.size(); i++) {
        total_width += col_widths[i] + 3;
    }

    if (draw_grid) {
        ret << std::string(total_width, ROW) << "\n";
    }

    for (size_t y = 0; y < data.size() / cols; y++) {
        for (size_t line_i = 0; line_i < row_heights[y]; line_i++ ) {
            for (size_t x = 0; x < cols; x++) {
                if (draw_grid && x == 0) {
                    ret << COL.substr(1);
                }
                if (draw_grid && x != 0) {
                    ret << COL;
                }
                ret << data_lines[x + y * cols][line_i];
                if (draw_grid && x == cols - 1) {
                    ret << COL.substr(0, 2);
                }
            }
            ret << "\n";
        }
        if (draw_grid) {
            ret << std::string(total_width, ROW) << "\n";
        }
    }

    return ret.str();
}

#if defined(DEBUG) && defined(PARANOID) //functions that cost more than constant time to check
template<class T>
void assert_permutation(const T& p, size_t n) {
    for(size_t i = 0; i < n; ++i)
    for(size_t j = 0; j < n; ++j)
    {
        if(i == j) continue;
        DCHECK_NE(p[i],p[j]) << "at positions " << i << " and " << j;
        DCHECK_LT(p[i],n);
    }
}
/** Checks whether p is a permutation that maps to the range [from,to) instead of [0..n)
 */
template<class T>
void assert_permutation_offset(const T& p, size_t n,size_t offset) {
    for(size_t i = 0; i < n; ++i)
    for(size_t j = 0; j < n; ++j)
    {
        if(i == j) continue;
        DCHECK_NE(p[i],p[j]) << "at positions " << i << " and " << j;
        DCHECK_GE(p[i],offset);
        DCHECK_LT(p[i],offset+n);
    }
}
#else
template<class T> inline void assert_permutation(const T&, size_t) {}
template<class T> inline void assert_permutation_offset(const T&, size_t,size_t) {}
#endif


/**
 * @brief Division with rounding up to the next integer.
 * Call equivalent to (int) std::ceil(((double)x)/y)
 * @param x dividend
 * @param y divisor
 * @return ceiling of the term x/y
 */
template<class T>
constexpr T round_up_div(T x, T y) {
	return (x + y -1)/y;
}

/*
 *	Square root by abacus algorithm, Martin Guy @ UKC, June 1985.
 *	From a book on programming abaci by Mr C. Woo.
 *  and from https://en.wikipedia.org/wiki/Methods_of_computing_square_roots
 */
template<class int_t>
int_t isqrt(int_t num) {
    int_t res = 0;
    int_t bit = 1ULL << (sizeof(int_t)-2); // The second-to-top bit is set: 1 << 30 for 32 bits

    // "bit" starts at the highest power of four <= the argument.
    while (bit > num)
        bit >>= 2;

    while (bit != 0) {
        if (num >= res + bit) {
            num -= res + bit;
            res = (res >> 1) + bit;
        }
        else
            res >>= 1;
        bit >>= 2;
    }
    return res;
}

static inline size_t lz78_expected_number_of_remaining_elements(const size_t z, const size_t n, const size_t remaining_characters) {
		if(remaining_characters*2 < n ) {
			return (z*remaining_characters) / (n - remaining_characters);
		}
	return remaining_characters*3/(bits_for(remaining_characters));
}

class MoveGuard {
    bool m_is_not_moved;
public:
    inline MoveGuard(): m_is_not_moved(true) {}
    inline MoveGuard(const MoveGuard& other) {
        DCHECK(other.m_is_not_moved) << "Trying to copy a already moved-from MoveGuard";
        m_is_not_moved = other.m_is_not_moved;
    }
    inline MoveGuard(MoveGuard&& other) {
        DCHECK(other.m_is_not_moved) << "Trying to move a already moved-from MoveGuard";
        m_is_not_moved = other.m_is_not_moved;
        other.m_is_not_moved = false;
    }

    inline bool is_not_moved() const {
        return m_is_not_moved;
    }
    inline bool is_moved() const {
        return !m_is_not_moved;
    }
    inline operator bool() const {
        return is_not_moved();
    }
    inline bool operator!() const {
        return is_moved();
    }
};

}//ns
