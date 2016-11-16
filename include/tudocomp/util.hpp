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

#include <tudocomp/util/View.hpp>

#include <sdsl/bits.hpp>

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
inline size_t bits_for(size_t n) {
    if(n == 0) {
        return 1U;
    } else {
        return sdsl::bits::hi(n) + 1; //TODO get rid of SDSL dependency
    }
}

/// \brief Performs an integer division with the result rounded up to the
/// next integer.
///
/// \param a The dividend.
/// \param b The divisor.
/// \return The quotient, rounded up to the next integer value.
inline size_t idiv_ceil(size_t a, size_t b) {
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
inline size_t bytes_for(size_t n) {
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

}

/// \cond INTERNAL
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

/// \endcond
//Check that p is a permutation of [0..n-1]

#ifndef NDEBUG
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
#else
template<class T> void assert_permutation(const T&, size_t) {}
#endif


#endif
