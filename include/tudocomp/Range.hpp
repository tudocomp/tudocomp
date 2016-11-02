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
    class TypeRange {
    private:
        const uint64_t m_max = std::numeric_limits<T>::max();

    public:
        inline TypeRange() {}
        inline uint64_t min() const { return 0; }
        inline uint64_t max() const { return m_max; }
    };

    template<uint64_t m_min = 0, uint64_t m_max = UINT64_MAX>
    class IntRange {
    public:
        inline IntRange() {}
        inline uint64_t min() const { return m_min; }
        inline uint64_t max() const { return m_max; }
    };

    using Int64Range = IntRange<0, UINT64_MAX>;
    using Int32Range = IntRange<0, UINT32_MAX>;
    using Int16Range = IntRange<0, UINT16_MAX>;
    using Int8Range = IntRange<0, UINT8_MAX>;
    using ByteRange = Int8Range;
    using BitRange = IntRange<0, 1>;

    class CharRange {
    public:
        inline CharRange() {}
        inline uint64_t min() const { return 0; }
        inline uint64_t max() const { return 255; }
    };
}

#endif
