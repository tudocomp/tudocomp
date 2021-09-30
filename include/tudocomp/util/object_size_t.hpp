#pragma once

#include <iostream>
#include <typeinfo>
#include <tuple>
#include <memory>

namespace tdc {
    /// Represents the total size of some object.
    ///
    /// For example, it can represent the number of bytes written to a file,
    /// or the total heap size of a object in memory.
    ///
    /// It is possible for a datastructure to not have a known size. In that
    /// case, this datastructure should contain the closest lower approximation
    /// of one, and `is_exact()` returns `false`.
    class object_size_t {
        size_t m_bytes = 0;
        bool m_has_unknown_parts = false;

        object_size_t() = default;
        object_size_t(size_t bytes, bool has_unknown_parts):
            m_bytes(bytes), m_has_unknown_parts(has_unknown_parts) {}
    public:
        inline static object_size_t empty() {
            return object_size_t(0, false);
        }
        inline static object_size_t exact(size_t size) {
            return object_size_t(size, false);
        }
        inline static object_size_t unknown_extra_data(size_t size) {
            return object_size_t(size, true);
        }

        inline object_size_t operator+(object_size_t const& other) const {
            return object_size_t(
                m_bytes + other.m_bytes,
                m_has_unknown_parts || other.m_has_unknown_parts);
        }
        inline object_size_t& operator+=(object_size_t const& other) {
            m_bytes += other.m_bytes;
            m_has_unknown_parts |= other.m_has_unknown_parts;
            return *this;
        }

        inline size_t size_in_bytes() const {
            return m_bytes;
        }

        inline double size_in_kibibytes() const {
            return double(m_bytes) / 1024.0;
        }

        inline double size_in_mebibytes() const {
            return double(m_bytes) / 1024.0 / 1024.0;
        }

        inline bool is_exact() const {
            return !m_has_unknown_parts;
        }

        inline friend std::ostream& operator<<(std::ostream& out, object_size_t const& v) {
            if (!v.is_exact()) {
                out << ">=";
            }
            out << v.size_in_kibibytes();
            out << " KiB";
            return out;
        }
    };
}
