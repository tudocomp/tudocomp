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

    const uint64_t m_mask;
    const uint64_t m_sign_bit;
    const uint8_t  m_width;

    inline saidx_t to_signed(uint64_t v, int p = 0) {
        const uint8_t lshift = 64 - m_width;

        saidx_t r;
        if (v & m_sign_bit) {
            r = int64_t(v | m_mask);
        } else {
            r = int64_t(v);
        }

        std::cout << "S Cast from " << v << " to " << r << "\n";
        if (p == 0) {
            std::cout << "Back ";
            auto r2 = to_unsigned(r, p + 1);

            std::cout << "v:  " << pm(v) << "\n";
            std::cout << "r2: " << pm(r2) << "\n";
            DCHECK_EQ(v, r2);
        }

        return r;
    }

    inline uint64_t to_unsigned(saidx_t v, int p = 0) {
        const uint8_t lshift = 64 - m_width;

        uint64_t r;
        r = int64_t(v);
        //r = r & (~m_mask);

        DCHECK_EQ(v, int64_t(v));

        std::cout << "U Cast from " << v << " to " << r << "\n";
        if (p == 0) {
            std::cout << "Back ";
            auto r2 = to_signed(r, p + 1);

            std::cout << "v:  " << pm(v) << "\n";
            std::cout << "r2: " << pm(r2) << "\n";
            DCHECK_EQ(v, r2);
        }

        return r;

        // NB: Could do this:
        // return int64_t(v & (~m_mask));
        // to properly truncate, but assigning to a DynamicIntVector
        // does this implicitly
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
        m_mask(~((1ull << buffer.width()) - 1ull)),
        m_sign_bit(1ull << (buffer.width() - 1)),
        m_width(buffer.width())
    {
        DCHECK_EQ(m_mask & m_sign_bit, m_sign_bit);
        DCHECK_EQ(m_mask & (m_sign_bit << 1), 0);
        DCHECK_EQ(m_mask & (m_sign_bit << 1), (m_sign_bit << 1));

        if (buffer.width() == 64) {
            DCHECK_EQ(m_mask, ~uint64_t(0));
            DCHECK_EQ(m_mask, uint64_t(-1));
        }

        std::cout << "width: " << int(buffer.width()) << "\n";
        std::cout << "mask:  " << pm(m_mask) << "\n";
        std::cout << "sign:  " << pm(m_sign_bit) << "\n";
    }

    inline operator saidx_t() { return to_signed(m_buffer[m_index]); }
    inline Accessor& operator=(saidx_t v) { m_buffer[m_index] = to_unsigned(v); return *this; }

    inline Accessor& operator=(const Accessor& other) {
        m_buffer[m_index] = other.m_buffer[other.m_index];
        return *this;
    }
};

}} //ns
