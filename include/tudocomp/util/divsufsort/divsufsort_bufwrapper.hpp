#pragma once

#include <tudocomp/util/divsufsort/divsufsort_def.hpp>
#include <tudocomp/ds/IntVector.hpp>

namespace tdc {
namespace libdivsufsort {

// Wrapper for buffers with unsigned value types
template<typename buffer_t>
class BufferWrapper {
private:
    buffer_t& m_buffer;

    class Accessor {
    private:
        buffer_t& m_buffer;
        saidx_t m_index;

    public:
        inline Accessor(buffer_t& buffer, saidx_t i) : m_buffer(buffer), m_index(i) {}
        inline operator saidx_t() { return saidx_t(m_buffer[m_index]); }
        inline Accessor& operator=(saidx_t v) { m_buffer[m_index] = v; return *this; }

        inline Accessor& operator=(const Accessor& other) {
            m_buffer[m_index] = other.m_buffer[other.m_index];
            return *this;
        }
    };

public:
    BufferWrapper(buffer_t& buffer) : m_buffer(buffer) {}
    inline Accessor operator[](saidx_t i) { return Accessor(m_buffer, i); }
};

// special accessor for DynamicIntVector, where sign casts are not that simple
template<>
class BufferWrapper<DynamicIntVector>::Accessor {
private:
    DynamicIntVector& m_buffer;
    saidx_t m_index;

    // NB: Could add a bit mask for the maximum value width here,
    // such that a int64_t representation only uses DynamicIntVector::width bits(),
    // but a DynamicIntVector silently truncates the values, so its not needed.
    // mask = ~(1ull << buffer.width())
    // val = val & mask

    inline saidx_t to_signed(uint64_t v) {
        return int64_t(v);
    }

    inline uint64_t to_unsigned(saidx_t v) {
        return int64_t(v);
    }

public:
    inline Accessor(DynamicIntVector& buffer, saidx_t i)
        : m_buffer(buffer), m_index(i)
    {
    }

    inline operator saidx_t() { return to_signed(m_buffer[m_index]); }
    inline Accessor& operator=(saidx_t v) { m_buffer[m_index] = to_unsigned(v); return *this; }

    inline Accessor& operator=(const Accessor& other) {
        m_buffer[m_index] = other.m_buffer[other.m_index];
        return *this;
    }
};

}} //ns
