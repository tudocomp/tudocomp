#ifndef TUDOCOMP_INTEGER_OPERATOR_BASE_H
#define TUDOCOMP_INTEGER_OPERATOR_BASE_H

namespace tdc {

static_assert(sizeof(int) * 8 == 32, "Make sure the logic here remains correct");

template<class T, class X = void>
struct IntegerBaseTrait {
    typedef uint64_t SelfMaxBit;

    inline static void assign(T& self, uint32_t v) {}
    inline static void assign(T& self, uint64_t v) {}
    inline static SelfMaxBit cast_for_self_op(const T& self) { return 0; }
    inline static SelfMaxBit cast_for_32_op(const T& self)   { return 0; }
    inline static uint64_t cast_for_64_op(const T& self)     { return 0; }
};

template<class Self>
class IntegerBase;

template<class Self>
class ConstIntegerBase {
public:
    typedef typename IntegerBaseTrait<Self>::SelfMaxBit SelfMaxBit;
private:
    inline static SelfMaxBit cast_for_self_op(const Self& self) {
        return IntegerBaseTrait<Self>::cast_for_self_op(self);
    }

    inline static uint64_t cast_for_64_op(const Self& self) {
        return IntegerBaseTrait<Self>::cast_for_64_op(self);
    }

    inline static SelfMaxBit cast_for_32_op(const Self& self) {
        return IntegerBaseTrait<Self>::cast_for_32_op(self);
    }

    friend class IntegerBase<Self>;
public:
    friend SelfMaxBit operator+(const Self& lhs, const Self& rhs)     { return cast_for_self_op(lhs) + cast_for_self_op(rhs); }
    friend SelfMaxBit operator+(const uint32_t& lhs, const Self& rhs) { return lhs                   + cast_for_32_op(rhs);   }
    friend SelfMaxBit operator+(const Self& lhs, const uint32_t& rhs) { return cast_for_32_op(lhs)   + rhs;                   }
    friend uint64_t   operator+(const uint64_t& lhs, const Self& rhs) { return lhs                   + cast_for_64_op(rhs);   }
    friend uint64_t   operator+(const Self& lhs, const uint64_t& rhs) { return cast_for_64_op(lhs)   + rhs;                   }
    friend SelfMaxBit operator+(const int& lhs, const Self& rhs)      { return lhs                   + cast_for_32_op(rhs);   }
    friend SelfMaxBit operator+(const Self& lhs, const int& rhs)      { return cast_for_32_op(lhs)   + rhs;                   }

    friend SelfMaxBit operator-(const Self& lhs, const Self& rhs)     { return cast_for_self_op(lhs) - cast_for_self_op(rhs); }
    friend SelfMaxBit operator-(const uint32_t& lhs, const Self& rhs) { return lhs                   - cast_for_32_op(rhs);   }
    friend SelfMaxBit operator-(const Self& lhs, const uint32_t& rhs) { return cast_for_32_op(lhs)   - rhs;                   }
    friend uint64_t   operator-(const uint64_t& lhs, const Self& rhs) { return lhs                   - cast_for_64_op(rhs);   }
    friend uint64_t   operator-(const Self& lhs, const uint64_t& rhs) { return cast_for_64_op(lhs)   - rhs;                   }
    friend SelfMaxBit operator-(const int& lhs, const Self& rhs)      { return lhs                   - cast_for_32_op(rhs);   }
    friend SelfMaxBit operator-(const Self& lhs, const int& rhs)      { return cast_for_32_op(lhs)   - rhs;                   }

    friend SelfMaxBit operator*(const Self& lhs, const Self& rhs)     { return cast_for_self_op(lhs) * cast_for_self_op(rhs); }
    friend SelfMaxBit operator*(const uint32_t& lhs, const Self& rhs) { return lhs                   * cast_for_32_op(rhs);   }
    friend SelfMaxBit operator*(const Self& lhs, const uint32_t& rhs) { return cast_for_32_op(lhs)   * rhs;                   }
    friend uint64_t   operator*(const uint64_t& lhs, const Self& rhs) { return lhs                   * cast_for_64_op(rhs);   }
    friend uint64_t   operator*(const Self& lhs, const uint64_t& rhs) { return cast_for_64_op(lhs)   * rhs;                   }
    friend SelfMaxBit operator*(const int& lhs, const Self& rhs)      { return lhs                   * cast_for_32_op(rhs);   }
    friend SelfMaxBit operator*(const Self& lhs, const int& rhs)      { return cast_for_32_op(lhs)   * rhs;                   }

    friend SelfMaxBit operator/(const Self& lhs, const Self& rhs)     { return cast_for_self_op(lhs) / cast_for_self_op(rhs); }
    friend SelfMaxBit operator/(const uint32_t& lhs, const Self& rhs) { return lhs                   / cast_for_32_op(rhs);   }
    friend SelfMaxBit operator/(const Self& lhs, const uint32_t& rhs) { return cast_for_32_op(lhs)   / rhs;                   }
    friend uint64_t   operator/(const uint64_t& lhs, const Self& rhs) { return lhs                   / cast_for_64_op(rhs);   }
    friend uint64_t   operator/(const Self& lhs, const uint64_t& rhs) { return cast_for_64_op(lhs)   / rhs;                   }
    friend SelfMaxBit operator/(const int& lhs, const Self& rhs)      { return lhs                   / cast_for_32_op(rhs);   }
    friend SelfMaxBit operator/(const Self& lhs, const int& rhs)      { return cast_for_32_op(lhs)   / rhs;                   }

    friend SelfMaxBit operator%(const Self& lhs, const Self& rhs)     { return cast_for_self_op(lhs) % cast_for_self_op(rhs); }
    friend SelfMaxBit operator%(const uint32_t& lhs, const Self& rhs) { return lhs                   % cast_for_32_op(rhs);   }
    friend SelfMaxBit operator%(const Self& lhs, const uint32_t& rhs) { return cast_for_32_op(lhs)   % rhs;                   }
    friend uint64_t   operator%(const uint64_t& lhs, const Self& rhs) { return lhs                   % cast_for_64_op(rhs);   }
    friend uint64_t   operator%(const Self& lhs, const uint64_t& rhs) { return cast_for_64_op(lhs)   % rhs;                   }
    friend SelfMaxBit operator%(const int& lhs, const Self& rhs)      { return lhs                   % cast_for_32_op(rhs);   }
    friend SelfMaxBit operator%(const Self& lhs, const int& rhs)      { return cast_for_32_op(lhs)   % rhs;                   }

    friend SelfMaxBit operator&(const Self& lhs, const Self& rhs)     { return cast_for_self_op(lhs) & cast_for_self_op(rhs); }
    friend SelfMaxBit operator&(const uint32_t& lhs, const Self& rhs) { return lhs                   & cast_for_32_op(rhs);   }
    friend SelfMaxBit operator&(const Self& lhs, const uint32_t& rhs) { return cast_for_32_op(lhs)   & rhs;                   }
    friend uint64_t   operator&(const uint64_t& lhs, const Self& rhs) { return lhs                   & cast_for_64_op(rhs);   }
    friend uint64_t   operator&(const Self& lhs, const uint64_t& rhs) { return cast_for_64_op(lhs)   & rhs;                   }
    friend SelfMaxBit operator&(const int& lhs, const Self& rhs)      { return lhs                   & cast_for_32_op(rhs);   }
    friend SelfMaxBit operator&(const Self& lhs, const int& rhs)      { return cast_for_32_op(lhs)   & rhs;                   }

    friend SelfMaxBit operator|(const Self& lhs, const Self& rhs)     { return cast_for_self_op(lhs) | cast_for_self_op(rhs); }
    friend SelfMaxBit operator|(const uint32_t& lhs, const Self& rhs) { return lhs                   | cast_for_32_op(rhs);   }
    friend SelfMaxBit operator|(const Self& lhs, const uint32_t& rhs) { return cast_for_32_op(lhs)   | rhs;                   }
    friend uint64_t   operator|(const uint64_t& lhs, const Self& rhs) { return lhs                   | cast_for_64_op(rhs);   }
    friend uint64_t   operator|(const Self& lhs, const uint64_t& rhs) { return cast_for_64_op(lhs)   | rhs;                   }
    friend SelfMaxBit operator|(const int& lhs, const Self& rhs)      { return lhs                   | cast_for_32_op(rhs);   }
    friend SelfMaxBit operator|(const Self& lhs, const int& rhs)      { return cast_for_32_op(lhs)   | rhs;                   }

    friend SelfMaxBit operator^(const Self& lhs, const Self& rhs)     { return cast_for_self_op(lhs) ^ cast_for_self_op(rhs); }
    friend SelfMaxBit operator^(const uint32_t& lhs, const Self& rhs) { return lhs                   ^ cast_for_32_op(rhs);   }
    friend SelfMaxBit operator^(const Self& lhs, const uint32_t& rhs) { return cast_for_32_op(lhs)   ^ rhs;                   }
    friend uint64_t   operator^(const uint64_t& lhs, const Self& rhs) { return lhs                   ^ cast_for_64_op(rhs);   }
    friend uint64_t   operator^(const Self& lhs, const uint64_t& rhs) { return cast_for_64_op(lhs)   ^ rhs;                   }
    friend SelfMaxBit operator^(const int& lhs, const Self& rhs)      { return lhs                   ^ cast_for_32_op(rhs);   }
    friend SelfMaxBit operator^(const Self& lhs, const int& rhs)      { return cast_for_32_op(lhs)   ^ rhs;                   }

    friend SelfMaxBit operator~(const Self& self) { return ~(cast_for_self_op(self)); }

    friend SelfMaxBit operator<<(const Self& lhs, const Self& rhs)     { return cast_for_self_op(lhs) << cast_for_self_op(rhs); }
    friend SelfMaxBit operator<<(const uint32_t& lhs, const Self& rhs) { return lhs                   << cast_for_32_op(rhs);   }
    friend SelfMaxBit operator<<(const Self& lhs, const uint32_t& rhs) { return cast_for_32_op(lhs)   << rhs;                   }
    friend uint64_t   operator<<(const uint64_t& lhs, const Self& rhs) { return lhs                   << cast_for_64_op(rhs);   }
    friend uint64_t   operator<<(const Self& lhs, const uint64_t& rhs) { return cast_for_64_op(lhs)   << rhs;                   }
    friend SelfMaxBit operator<<(const int& lhs, const Self& rhs)      { return lhs                   << cast_for_32_op(rhs);   }
    friend SelfMaxBit operator<<(const Self& lhs, const int& rhs)      { return cast_for_32_op(lhs)   << rhs;                   }

    friend SelfMaxBit operator>>(const Self& lhs, const Self& rhs)     { return cast_for_self_op(lhs) >> cast_for_self_op(rhs); }
    friend SelfMaxBit operator>>(const uint32_t& lhs, const Self& rhs) { return lhs                   >> cast_for_32_op(rhs);   }
    friend SelfMaxBit operator>>(const Self& lhs, const uint32_t& rhs) { return cast_for_32_op(lhs)   >> rhs;                   }
    friend uint64_t   operator>>(const uint64_t& lhs, const Self& rhs) { return lhs                   >> cast_for_64_op(rhs);   }
    friend uint64_t   operator>>(const Self& lhs, const uint64_t& rhs) { return cast_for_64_op(lhs)   >> rhs;                   }
    friend SelfMaxBit operator>>(const int& lhs, const Self& rhs)      { return lhs                   >> cast_for_32_op(rhs);   }
    friend SelfMaxBit operator>>(const Self& lhs, const int& rhs)      { return cast_for_32_op(lhs)   >> rhs;                   }

    friend bool operator==(const Self& lhs, const Self& rhs)     { return cast_for_self_op(lhs) == cast_for_self_op(rhs); }
    friend bool operator==(const uint32_t& lhs, const Self& rhs) { return lhs                   == cast_for_32_op(rhs);   }
    friend bool operator==(const Self& lhs, const uint32_t& rhs) { return cast_for_32_op(lhs)   == rhs;                   }
    friend bool operator==(const uint64_t& lhs, const Self& rhs) { return lhs                   == cast_for_64_op(rhs);   }
    friend bool operator==(const Self& lhs, const uint64_t& rhs) { return cast_for_64_op(lhs)   == rhs;                   }
    friend bool operator==(const int& lhs, const Self& rhs)      { return lhs                   == cast_for_32_op(rhs);   }
    friend bool operator==(const Self& lhs, const int& rhs)      { return cast_for_32_op(lhs)   == rhs;                   }

    friend bool operator!=(const Self& lhs, const Self& rhs)     { return cast_for_self_op(lhs) != cast_for_self_op(rhs); }
    friend bool operator!=(const uint32_t& lhs, const Self& rhs) { return lhs                   != cast_for_32_op(rhs);   }
    friend bool operator!=(const Self& lhs, const uint32_t& rhs) { return cast_for_32_op(lhs)   != rhs;                   }
    friend bool operator!=(const uint64_t& lhs, const Self& rhs) { return lhs                   != cast_for_64_op(rhs);   }
    friend bool operator!=(const Self& lhs, const uint64_t& rhs) { return cast_for_64_op(lhs)   != rhs;                   }
    friend bool operator!=(const int& lhs, const Self& rhs)      { return lhs                   != cast_for_32_op(rhs);   }
    friend bool operator!=(const Self& lhs, const int& rhs)      { return cast_for_32_op(lhs)   != rhs;                   }

    friend bool operator>(const Self& lhs, const Self& rhs)     { return cast_for_self_op(lhs) > cast_for_self_op(rhs); }
    friend bool operator>(const uint32_t& lhs, const Self& rhs) { return lhs                   > cast_for_32_op(rhs);   }
    friend bool operator>(const Self& lhs, const uint32_t& rhs) { return cast_for_32_op(lhs)   > rhs;                   }
    friend bool operator>(const uint64_t& lhs, const Self& rhs) { return lhs                   > cast_for_64_op(rhs);   }
    friend bool operator>(const Self& lhs, const uint64_t& rhs) { return cast_for_64_op(lhs)   > rhs;                   }
    friend bool operator>(const int& lhs, const Self& rhs)      { return lhs                   > cast_for_32_op(rhs);   }
    friend bool operator>(const Self& lhs, const int& rhs)      { return cast_for_32_op(lhs)   > rhs;                   }

    friend bool operator<(const Self& lhs, const Self& rhs)     { return cast_for_self_op(lhs) < cast_for_self_op(rhs); }
    friend bool operator<(const uint32_t& lhs, const Self& rhs) { return lhs                   < cast_for_32_op(rhs);   }
    friend bool operator<(const Self& lhs, const uint32_t& rhs) { return cast_for_32_op(lhs)   < rhs;                   }
    friend bool operator<(const uint64_t& lhs, const Self& rhs) { return lhs                   < cast_for_64_op(rhs);   }
    friend bool operator<(const Self& lhs, const uint64_t& rhs) { return cast_for_64_op(lhs)   < rhs;                   }
    friend bool operator<(const int& lhs, const Self& rhs)      { return lhs                   < cast_for_32_op(rhs);   }
    friend bool operator<(const Self& lhs, const int& rhs)      { return cast_for_32_op(lhs)   < rhs;                   }

    friend bool operator>=(const Self& lhs, const Self& rhs)     { return cast_for_self_op(lhs) >= cast_for_self_op(rhs); }
    friend bool operator>=(const uint32_t& lhs, const Self& rhs) { return lhs                   >= cast_for_32_op(rhs);   }
    friend bool operator>=(const Self& lhs, const uint32_t& rhs) { return cast_for_32_op(lhs)   >= rhs;                   }
    friend bool operator>=(const uint64_t& lhs, const Self& rhs) { return lhs                   >= cast_for_64_op(rhs);   }
    friend bool operator>=(const Self& lhs, const uint64_t& rhs) { return cast_for_64_op(lhs)   >= rhs;                   }
    friend bool operator>=(const int& lhs, const Self& rhs)      { return lhs                   >= cast_for_32_op(rhs);   }
    friend bool operator>=(const Self& lhs, const int& rhs)      { return cast_for_32_op(lhs)   >= rhs;                   }

    friend bool operator<=(const Self& lhs, const Self& rhs)     { return cast_for_self_op(lhs) <= cast_for_self_op(rhs); }
    friend bool operator<=(const uint32_t& lhs, const Self& rhs) { return lhs                   <= cast_for_32_op(rhs);   }
    friend bool operator<=(const Self& lhs, const uint32_t& rhs) { return cast_for_32_op(lhs)   <= rhs;                   }
    friend bool operator<=(const uint64_t& lhs, const Self& rhs) { return lhs                   <= cast_for_64_op(rhs);   }
    friend bool operator<=(const Self& lhs, const uint64_t& rhs) { return cast_for_64_op(lhs)   <= rhs;                   }
    friend bool operator<=(const int& lhs, const Self& rhs)      { return lhs                   <= cast_for_32_op(rhs);   }
    friend bool operator<=(const Self& lhs, const int& rhs)      { return cast_for_32_op(lhs)   <= rhs;                   }

};

template<class Self>
class IntegerBase: public ConstIntegerBase<Self> {
private:
    typedef typename ConstIntegerBase<Self>::SelfMaxBit SelfMaxBit;
    using ConstIntegerBase<Self>::cast_for_self_op;
    using ConstIntegerBase<Self>::cast_for_32_op;
    using ConstIntegerBase<Self>::cast_for_64_op;

    inline static void assign(Self& self, uint32_t v) {
        IntegerBaseTrait<Self>::assign(self, v);
    }

    inline static void assign(Self& self, uint64_t v) {
        IntegerBaseTrait<Self>::assign(self, v);
    }
public:
    Self& operator+=(const Self& v)     { auto& self = static_cast<Self&>(*this); assign(self, self + v); return self; }
    Self& operator+=(const uint32_t& v) { auto& self = static_cast<Self&>(*this); assign(self, self + v); return self; }
    Self& operator+=(const uint64_t& v) { auto& self = static_cast<Self&>(*this); assign(self, self + v); return self; }
    Self& operator+=(const int& v)      { auto& self = static_cast<Self&>(*this); assign(self, self + v); return self; }

    Self& operator-=(const Self& v)     { auto& self = static_cast<Self&>(*this); assign(self, self - v); return self; }
    Self& operator-=(const uint32_t& v) { auto& self = static_cast<Self&>(*this); assign(self, self - v); return self; }
    Self& operator-=(const uint64_t& v) { auto& self = static_cast<Self&>(*this); assign(self, self - v); return self; }
    Self& operator-=(const int& v)      { auto& self = static_cast<Self&>(*this); assign(self, self - v); return self; }

    Self& operator*=(const Self& v)     { auto& self = static_cast<Self&>(*this); assign(self, self * v); return self; }
    Self& operator*=(const uint32_t& v) { auto& self = static_cast<Self&>(*this); assign(self, self * v); return self; }
    Self& operator*=(const uint64_t& v) { auto& self = static_cast<Self&>(*this); assign(self, self * v); return self; }
    Self& operator*=(const int& v)      { auto& self = static_cast<Self&>(*this); assign(self, self * v); return self; }

    Self& operator/=(const Self& v)     { auto& self = static_cast<Self&>(*this); assign(self, self / v); return self; }
    Self& operator/=(const uint32_t& v) { auto& self = static_cast<Self&>(*this); assign(self, self / v); return self; }
    Self& operator/=(const uint64_t& v) { auto& self = static_cast<Self&>(*this); assign(self, self / v); return self; }
    Self& operator/=(const int& v)      { auto& self = static_cast<Self&>(*this); assign(self, self / v); return self; }

    Self& operator%=(const Self& v)     { auto& self = static_cast<Self&>(*this); assign(self, self % v); return self; }
    Self& operator%=(const uint32_t& v) { auto& self = static_cast<Self&>(*this); assign(self, self % v); return self; }
    Self& operator%=(const uint64_t& v) { auto& self = static_cast<Self&>(*this); assign(self, self % v); return self; }
    Self& operator%=(const int& v)      { auto& self = static_cast<Self&>(*this); assign(self, self % v); return self; }

    Self& operator++() { auto& self = static_cast<Self&>(*this); assign(self, self + 1); return self; }
    Self& operator--() { auto& self = static_cast<Self&>(*this); assign(self, self - 1); return self; }

    SelfMaxBit operator++(int) { auto& self = static_cast<Self&>(*this); auto tmp = cast_for_self_op(self); assign(self, self + 1); return tmp; }
    SelfMaxBit operator--(int) { auto& self = static_cast<Self&>(*this); auto tmp = cast_for_self_op(self); assign(self, self - 1); return tmp; }

    Self& operator&=(const Self& v)     { auto& self = static_cast<Self&>(*this); assign(self, self & v); return self; }
    Self& operator&=(const uint32_t& v) { auto& self = static_cast<Self&>(*this); assign(self, self & v); return self; }
    Self& operator&=(const uint64_t& v) { auto& self = static_cast<Self&>(*this); assign(self, self & v); return self; }
    Self& operator&=(const int& v)      { auto& self = static_cast<Self&>(*this); assign(self, self & v); return self; }

    Self& operator|=(const Self& v)     { auto& self = static_cast<Self&>(*this); assign(self, self | v); return self; }
    Self& operator|=(const uint32_t& v) { auto& self = static_cast<Self&>(*this); assign(self, self | v); return self; }
    Self& operator|=(const uint64_t& v) { auto& self = static_cast<Self&>(*this); assign(self, self | v); return self; }
    Self& operator|=(const int& v)      { auto& self = static_cast<Self&>(*this); assign(self, self | v); return self; }

    Self& operator^=(const Self& v)     { auto& self = static_cast<Self&>(*this); assign(self, self ^ v); return self; }
    Self& operator^=(const uint32_t& v) { auto& self = static_cast<Self&>(*this); assign(self, self ^ v); return self; }
    Self& operator^=(const uint64_t& v) { auto& self = static_cast<Self&>(*this); assign(self, self ^ v); return self; }
    Self& operator^=(const int& v)      { auto& self = static_cast<Self&>(*this); assign(self, self ^ v); return self; }

    Self& operator>>=(const Self& v)     { auto& self = static_cast<Self&>(*this); assign(self, self >> v); return self; }
    Self& operator>>=(const uint32_t& v) { auto& self = static_cast<Self&>(*this); assign(self, self >> v); return self; }
    Self& operator>>=(const uint64_t& v) { auto& self = static_cast<Self&>(*this); assign(self, self >> v); return self; }
    Self& operator>>=(const int& v)      { auto& self = static_cast<Self&>(*this); assign(self, self >> v); return self; }

    Self& operator<<=(const Self& v)     { auto& self = static_cast<Self&>(*this); assign(self, self << v); return self; }
    Self& operator<<=(const uint32_t& v) { auto& self = static_cast<Self&>(*this); assign(self, self << v); return self; }
    Self& operator<<=(const uint64_t& v) { auto& self = static_cast<Self&>(*this); assign(self, self << v); return self; }
    Self& operator<<=(const int& v)      { auto& self = static_cast<Self&>(*this); assign(self, self << v); return self; }

};

}

#endif
