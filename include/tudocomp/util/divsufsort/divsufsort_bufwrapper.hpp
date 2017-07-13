#pragma once

#include <algorithm>

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

    const int m_shift;

    inline saidx_t to_signed(uint64_t v, int p = 0) {
        // NB: Need to use portable arithmetic shift instruction
        // to ensure shifting a signed value causes a proper sign extension
        return shift_by(shift_by(int64_t(v), m_shift), -m_shift);
    }

    inline uint64_t to_unsigned(saidx_t v, int p = 0) {
        return (uint64_t(int64_t(v)) << m_shift) >> m_shift;
    }

    std::string pm(uint64_t v) {
        std::stringstream ss;
        for(size_t i = 0; i < 64; i++) {
            ss << int((v & (1ull << (64 - i - 1))) != 0);
        }
        return ss.str();
    }

public:
    inline Accessor(DynamicIntVector& buffer, saidx_t i):
        m_buffer(buffer),
        m_index(i),
        m_shift(64 - buffer.width())
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
