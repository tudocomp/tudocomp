#ifndef _INCLUDED_RANGE_HPP
#define _INCLUDED_RANGE_HPP

#include <tudocomp/def.hpp>
#include <limits>

namespace tdc {
    class Range {
    private:
        size_t m_min, m_max;

    public:
        inline Range(size_t max) : m_min(0), m_max(max) {}
        inline Range(size_t min, size_t max) : m_min(min), m_max(max) {}
        inline size_t min() const { return m_min; }
        inline size_t max() const { return m_max; }
        inline size_t delta() const { return m_max - m_min; }
    };

    class MinDistributedRange : public Range {
    public:
        inline MinDistributedRange(size_t max) : Range(0, max) {}
        inline MinDistributedRange(size_t min, size_t max) : Range(min, max) {}
    };

    template<typename T>
    class TypeRange : public Range {
    public:
        TypeRange() : Range(0, std::numeric_limits<T>::max()) {}
    };

    template<size_t t_min, size_t t_max>
    class FixedRange : public Range {
    public:
        FixedRange() : Range(t_min, t_max)  {}
    };

    class LiteralRange : public TypeRange<uliteral_t> {};
    class LengthRange  : public TypeRange<len_t> {};

    using BitRange = FixedRange<0, 1>;

    const TypeRange<size_t>   size_r;
    const BitRange bit_r;
    const LiteralRange literal_r;
    const LengthRange len_r;
}

#endif

