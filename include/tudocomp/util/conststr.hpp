// string literal class based on
// http://en.cppreference.com/w/cpp/language/constexpr

#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>

class conststr {
private:
    static constexpr const char* EMPTY = "";

    const char* m_ptr;
    size_t m_size;

public:
    inline constexpr conststr() : m_ptr(EMPTY), m_size(0) {
    }

    template<size_t N>
    inline constexpr conststr(const char(&a)[N]) : m_ptr(a), m_size(N - 1) {
    }

    inline constexpr conststr(const conststr& other)
        : m_ptr(other.m_ptr), m_size(other.m_size) {
    }

    inline constexpr char operator[](size_t n) const {
        return n < m_size ? m_ptr[n] : throw std::out_of_range("");
    }

    inline constexpr const char* c_str() const {
        return m_ptr;
    }

    inline std::string str() const {
        return std::string(m_ptr, m_size);
    }

    inline constexpr size_t size() const {
        return m_size;
    }

    inline constexpr bool operator==(const conststr& other) const {
        if(m_size != other.m_size) return false;

        for(size_t i = 0; i < m_size; ++i) {
            if(m_ptr[i] != other.m_ptr[i]) return false;
        }
        return true;
    }

    inline constexpr bool operator!=(const conststr& other) const {
        return !(*this == other);
    }
};

#include <functional>

// std::hash support
namespace std
{
    template<> struct hash<conststr> {
        size_t operator()(const conststr& s) const noexcept {
            return std::hash<const char*>{}(s.c_str());
        }
    };
}

