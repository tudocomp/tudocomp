#ifndef _INCLUDED_RANGE_HPP
#define _INCLUDED_RANGE_HPP

#include <limits>

namespace tdc {
    class Range {
    private:
        uint64_t m_min, m_max;

    public:
        inline Range(uint64_t max) : m_min(0), m_max(max) {}
        inline Range(uint64_t min, uint64_t max) : m_min(min), m_max(max) {}
        inline uint64_t min() const { return m_min; }
        inline uint64_t max() const { return m_max; }
    };

    template<typename T>
    class TypeRange : public Range {
    public:
        TypeRange() : Range(0, std::numeric_limits<T>::max()) {}
        //inline uint64_t min() const { return 0; }
        //inline uint64_t max() const { return std::numeric_limits<T>::max(); }
    };

    template<uint64_t t_min, uint64_t t_max>
    class FixedRange : public Range {
    public:
        FixedRange() : Range(t_min, t_max)  {}
        //inline uint64_t min() const { return t_min; }
        //inline uint64_t max() const { return t_max; }
    };

    using BitRange = FixedRange<0, 1>;
    using LiteralRange = FixedRange<0, 255>;

    const TypeRange<uint64_t> uint64_r;
    const TypeRange<uint32_t> uint32_r;
    const TypeRange<uint16_t> uint16_r;
    const TypeRange<uint8_t>  uint8_r;
    const TypeRange<size_t>   size_r;
    const BitRange bit_r;
    const LiteralRange literal_r;
}

#endif

