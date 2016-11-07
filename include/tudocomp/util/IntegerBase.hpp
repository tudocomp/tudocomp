#ifndef TUDOCOMP_INTEGER_OPERATOR_BASE_H
#define TUDOCOMP_INTEGER_OPERATOR_BASE_H

namespace tdc {

static_assert(sizeof(int) * 8 == 32, "Make sure the logic here remains correct");

template<class T, class X = void>
struct ConstIntegerBaseTrait {
    typedef uint64_t SelfMaxBit;

    inline static SelfMaxBit cast_for_self_op(const T& self) { return 0; }
    inline static SelfMaxBit cast_for_32_op(const T& self)   { return 0; }
    inline static uint64_t cast_for_64_op(const T& self)     { return 0; }
};

template<class T, class X = void>
struct IntegerBaseTrait: public ConstIntegerBaseTrait<T, X> {
    inline static void assign(T& self, uint32_t v) {}
    inline static void assign(T& self, uint64_t v) {}
};

template<class Self>
class IntegerBaseWithSelf;
template<class Self, class Other>
class IntegerBaseWith32;
template<class Self, class Other>
class IntegerBaseWith64;

template<class Self, class Other>
class ConstIntegerBaseWith32 {
public:
    typedef typename ConstIntegerBaseTrait<Self>::SelfMaxBit SelfMaxBit;
private:
    inline static SelfMaxBit cast_for_32_op(const Self& self) {
        return ConstIntegerBaseTrait<Self>::cast_for_32_op(self);
    }

    friend class IntegerBaseWith32<Self, Other>;
public:
    friend SelfMaxBit operator+(const Other& lhs, const Self& rhs) { return lhs                   + cast_for_32_op(rhs);   }
    friend SelfMaxBit operator+(const Self& lhs, const Other& rhs) { return cast_for_32_op(lhs)   + rhs;                   }

    friend SelfMaxBit operator-(const Other& lhs, const Self& rhs) { return lhs                   - cast_for_32_op(rhs);   }
    friend SelfMaxBit operator-(const Self& lhs, const Other& rhs) { return cast_for_32_op(lhs)   - rhs;                   }

    friend SelfMaxBit operator*(const Other& lhs, const Self& rhs) { return lhs                   * cast_for_32_op(rhs);   }
    friend SelfMaxBit operator*(const Self& lhs, const Other& rhs) { return cast_for_32_op(lhs)   * rhs;                   }

    friend SelfMaxBit operator/(const Other& lhs, const Self& rhs) { return lhs                   / cast_for_32_op(rhs);   }
    friend SelfMaxBit operator/(const Self& lhs, const Other& rhs) { return cast_for_32_op(lhs)   / rhs;                   }

    friend SelfMaxBit operator%(const Other& lhs, const Self& rhs) { return lhs                   % cast_for_32_op(rhs);   }
    friend SelfMaxBit operator%(const Self& lhs, const Other& rhs) { return cast_for_32_op(lhs)   % rhs;                   }

    friend SelfMaxBit operator&(const Other& lhs, const Self& rhs) { return lhs                   & cast_for_32_op(rhs);   }
    friend SelfMaxBit operator&(const Self& lhs, const Other& rhs) { return cast_for_32_op(lhs)   & rhs;                   }

    friend SelfMaxBit operator|(const Other& lhs, const Self& rhs) { return lhs                   | cast_for_32_op(rhs);   }
    friend SelfMaxBit operator|(const Self& lhs, const Other& rhs) { return cast_for_32_op(lhs)   | rhs;                   }

    friend SelfMaxBit operator^(const Other& lhs, const Self& rhs) { return lhs                   ^ cast_for_32_op(rhs);   }
    friend SelfMaxBit operator^(const Self& lhs, const Other& rhs) { return cast_for_32_op(lhs)   ^ rhs;                   }

    friend SelfMaxBit operator<<(const Other& lhs, const Self& rhs) { return lhs                   << cast_for_32_op(rhs);   }
    friend SelfMaxBit operator<<(const Self& lhs, const Other& rhs) { return cast_for_32_op(lhs)   << rhs;                   }

    friend SelfMaxBit operator>>(const Other& lhs, const Self& rhs) { return lhs                   >> cast_for_32_op(rhs);   }
    friend SelfMaxBit operator>>(const Self& lhs, const Other& rhs) { return cast_for_32_op(lhs)   >> rhs;                   }

    friend bool operator==(const Other& lhs, const Self& rhs) { return lhs                   == cast_for_32_op(rhs);   }
    friend bool operator==(const Self& lhs, const Other& rhs) { return cast_for_32_op(lhs)   == rhs;                   }

    friend bool operator!=(const Other& lhs, const Self& rhs) { return lhs                   != cast_for_32_op(rhs);   }
    friend bool operator!=(const Self& lhs, const Other& rhs) { return cast_for_32_op(lhs)   != rhs;                   }

    friend bool operator>(const Other& lhs, const Self& rhs) { return lhs                   > cast_for_32_op(rhs);   }
    friend bool operator>(const Self& lhs, const Other& rhs) { return cast_for_32_op(lhs)   > rhs;                   }

    friend bool operator<(const Other& lhs, const Self& rhs) { return lhs                   < cast_for_32_op(rhs);   }
    friend bool operator<(const Self& lhs, const Other& rhs) { return cast_for_32_op(lhs)   < rhs;                   }

    friend bool operator>=(const Other& lhs, const Self& rhs) { return lhs                   >= cast_for_32_op(rhs);   }
    friend bool operator>=(const Self& lhs, const Other& rhs) { return cast_for_32_op(lhs)   >= rhs;                   }

    friend bool operator<=(const Other& lhs, const Self& rhs) { return lhs                   <= cast_for_32_op(rhs);   }
    friend bool operator<=(const Self& lhs, const Other& rhs) { return cast_for_32_op(lhs)   <= rhs;                   }
};

template<class Self, class Other>
class ConstIntegerBaseWith64 {
public:
    typedef typename ConstIntegerBaseTrait<Self>::SelfMaxBit SelfMaxBit;
private:
    inline static Other cast_for_64_op(const Self& self) {
        return ConstIntegerBaseTrait<Self>::cast_for_64_op(self);
    }

    friend class IntegerBaseWith64<Self, Other>;
public:
    friend Other   operator+(const Other& lhs, const Self& rhs) { return lhs                   + cast_for_64_op(rhs);   }
    friend Other   operator+(const Self& lhs, const Other& rhs) { return cast_for_64_op(lhs)   + rhs;                   }

    friend Other   operator-(const Other& lhs, const Self& rhs) { return lhs                   - cast_for_64_op(rhs);   }
    friend Other   operator-(const Self& lhs, const Other& rhs) { return cast_for_64_op(lhs)   - rhs;                   }

    friend Other   operator*(const Other& lhs, const Self& rhs) { return lhs                   * cast_for_64_op(rhs);   }
    friend Other   operator*(const Self& lhs, const Other& rhs) { return cast_for_64_op(lhs)   * rhs;                   }

    friend Other   operator/(const Other& lhs, const Self& rhs) { return lhs                   / cast_for_64_op(rhs);   }
    friend Other   operator/(const Self& lhs, const Other& rhs) { return cast_for_64_op(lhs)   / rhs;                   }

    friend Other   operator%(const Other& lhs, const Self& rhs) { return lhs                   % cast_for_64_op(rhs);   }
    friend Other   operator%(const Self& lhs, const Other& rhs) { return cast_for_64_op(lhs)   % rhs;                   }

    friend Other   operator&(const Other& lhs, const Self& rhs) { return lhs                   & cast_for_64_op(rhs);   }
    friend Other   operator&(const Self& lhs, const Other& rhs) { return cast_for_64_op(lhs)   & rhs;                   }

    friend Other   operator|(const Other& lhs, const Self& rhs) { return lhs                   | cast_for_64_op(rhs);   }
    friend Other   operator|(const Self& lhs, const Other& rhs) { return cast_for_64_op(lhs)   | rhs;                   }

    friend Other   operator^(const Other& lhs, const Self& rhs) { return lhs                   ^ cast_for_64_op(rhs);   }
    friend Other   operator^(const Self& lhs, const Other& rhs) { return cast_for_64_op(lhs)   ^ rhs;                   }

    friend Other   operator<<(const Other& lhs, const Self& rhs) { return lhs                   << cast_for_64_op(rhs);   }
    friend Other   operator<<(const Self& lhs, const Other& rhs) { return cast_for_64_op(lhs)   << rhs;                   }

    friend Other   operator>>(const Other& lhs, const Self& rhs) { return lhs                   >> cast_for_64_op(rhs);   }
    friend Other   operator>>(const Self& lhs, const Other& rhs) { return cast_for_64_op(lhs)   >> rhs;                   }

    friend bool operator==(const Other& lhs, const Self& rhs) { return lhs                   == cast_for_64_op(rhs);   }
    friend bool operator==(const Self& lhs, const Other& rhs) { return cast_for_64_op(lhs)   == rhs;                   }

    friend bool operator!=(const Other& lhs, const Self& rhs) { return lhs                   != cast_for_64_op(rhs);   }
    friend bool operator!=(const Self& lhs, const Other& rhs) { return cast_for_64_op(lhs)   != rhs;                   }

    friend bool operator>(const Other& lhs, const Self& rhs) { return lhs                   > cast_for_64_op(rhs);   }
    friend bool operator>(const Self& lhs, const Other& rhs) { return cast_for_64_op(lhs)   > rhs;                   }

    friend bool operator<(const Other& lhs, const Self& rhs) { return lhs                   < cast_for_64_op(rhs);   }
    friend bool operator<(const Self& lhs, const Other& rhs) { return cast_for_64_op(lhs)   < rhs;                   }

    friend bool operator>=(const Other& lhs, const Self& rhs) { return lhs                   >= cast_for_64_op(rhs);   }
    friend bool operator>=(const Self& lhs, const Other& rhs) { return cast_for_64_op(lhs)   >= rhs;                   }

    friend bool operator<=(const Other& lhs, const Self& rhs) { return lhs                   <= cast_for_64_op(rhs);   }
    friend bool operator<=(const Self& lhs, const Other& rhs) { return cast_for_64_op(lhs)   <= rhs;                   }
};

template<class Self>
class ConstIntegerBaseWithSelf {
public:
    typedef typename ConstIntegerBaseTrait<Self>::SelfMaxBit SelfMaxBit;
private:
    inline static SelfMaxBit cast_for_self_op(const Self& self) {
        return ConstIntegerBaseTrait<Self>::cast_for_self_op(self);
    }

    friend class IntegerBaseWithSelf<Self>;
public:
    friend SelfMaxBit operator+(const Self& lhs, const Self& rhs) { return cast_for_self_op(lhs) + cast_for_self_op(rhs); }
    friend SelfMaxBit operator-(const Self& lhs, const Self& rhs) { return cast_for_self_op(lhs) - cast_for_self_op(rhs); }
    friend SelfMaxBit operator*(const Self& lhs, const Self& rhs) { return cast_for_self_op(lhs) * cast_for_self_op(rhs); }
    friend SelfMaxBit operator/(const Self& lhs, const Self& rhs) { return cast_for_self_op(lhs) / cast_for_self_op(rhs); }
    friend SelfMaxBit operator%(const Self& lhs, const Self& rhs) { return cast_for_self_op(lhs) % cast_for_self_op(rhs); }
    friend SelfMaxBit operator&(const Self& lhs, const Self& rhs) { return cast_for_self_op(lhs) & cast_for_self_op(rhs); }
    friend SelfMaxBit operator|(const Self& lhs, const Self& rhs) { return cast_for_self_op(lhs) | cast_for_self_op(rhs); }
    friend SelfMaxBit operator^(const Self& lhs, const Self& rhs) { return cast_for_self_op(lhs) ^ cast_for_self_op(rhs); }
    friend SelfMaxBit operator~(const Self& self) { return ~(cast_for_self_op(self)); }
    friend SelfMaxBit operator<<(const Self& lhs, const Self& rhs) { return cast_for_self_op(lhs) << cast_for_self_op(rhs); }
    friend SelfMaxBit operator>>(const Self& lhs, const Self& rhs) { return cast_for_self_op(lhs) >> cast_for_self_op(rhs); }
    friend bool operator==(const Self& lhs, const Self& rhs) { return cast_for_self_op(lhs) == cast_for_self_op(rhs); }
    friend bool operator!=(const Self& lhs, const Self& rhs) { return cast_for_self_op(lhs) != cast_for_self_op(rhs); }
    friend bool operator>(const Self& lhs, const Self& rhs) { return cast_for_self_op(lhs) > cast_for_self_op(rhs); }
    friend bool operator<(const Self& lhs, const Self& rhs) { return cast_for_self_op(lhs) < cast_for_self_op(rhs); }
    friend bool operator>=(const Self& lhs, const Self& rhs) { return cast_for_self_op(lhs) >= cast_for_self_op(rhs); }
    friend bool operator<=(const Self& lhs, const Self& rhs) { return cast_for_self_op(lhs) <= cast_for_self_op(rhs); }
};

template<typename...>
class ConstIntegerBaseCombiner;

template<typename T, typename... Ts>
class ConstIntegerBaseCombiner<T, Ts...>: public T, public ConstIntegerBaseCombiner<Ts...> {
};

template<typename T>
class ConstIntegerBaseCombiner<T>: public T {
};

template<class Self>
using ConstIntegerBase = ConstIntegerBaseCombiner<
    ConstIntegerBaseWithSelf<Self>,
    ConstIntegerBaseWith32<Self, uint32_t>,
    ConstIntegerBaseWith32<Self, int>,
    ConstIntegerBaseWith64<Self, uint64_t>
>;

template<class Self, class Other>
class IntegerBaseWith32: public ConstIntegerBaseWith32<Self, Other> {
private:
    inline static void assign(Self& self, uint32_t v) {
        IntegerBaseTrait<Self>::assign(self, v);
    }

    inline static void assign(Self& self, uint64_t v) {
        IntegerBaseTrait<Self>::assign(self, v);
    }
public:
    Self& operator+=(const Other& v) { auto& self = static_cast<Self&>(*this); assign(self, self + v); return self; }
    Self& operator-=(const Other& v) { auto& self = static_cast<Self&>(*this); assign(self, self - v); return self; }
    Self& operator*=(const Other& v) { auto& self = static_cast<Self&>(*this); assign(self, self * v); return self; }
    Self& operator/=(const Other& v) { auto& self = static_cast<Self&>(*this); assign(self, self / v); return self; }
    Self& operator%=(const Other& v) { auto& self = static_cast<Self&>(*this); assign(self, self % v); return self; }
    Self& operator&=(const Other& v) { auto& self = static_cast<Self&>(*this); assign(self, self & v); return self; }
    Self& operator|=(const Other& v) { auto& self = static_cast<Self&>(*this); assign(self, self | v); return self; }
    Self& operator^=(const Other& v) { auto& self = static_cast<Self&>(*this); assign(self, self ^ v); return self; }
    Self& operator>>=(const Other& v) { auto& self = static_cast<Self&>(*this); assign(self, self >> v); return self; }
    Self& operator<<=(const Other& v) { auto& self = static_cast<Self&>(*this); assign(self, self << v); return self; }
};

template<class Self, class Other>
class IntegerBaseWith64: public ConstIntegerBaseWith64<Self, Other> {
private:
    inline static void assign(Self& self, uint32_t v) {
        IntegerBaseTrait<Self>::assign(self, v);
    }

    inline static void assign(Self& self, uint64_t v) {
        IntegerBaseTrait<Self>::assign(self, v);
    }
public:
    Self& operator+=(const Other& v) { auto& self = static_cast<Self&>(*this); assign(self, self + v); return self; }
    Self& operator-=(const Other& v) { auto& self = static_cast<Self&>(*this); assign(self, self - v); return self; }
    Self& operator*=(const Other& v) { auto& self = static_cast<Self&>(*this); assign(self, self * v); return self; }
    Self& operator/=(const Other& v) { auto& self = static_cast<Self&>(*this); assign(self, self / v); return self; }
    Self& operator%=(const Other& v) { auto& self = static_cast<Self&>(*this); assign(self, self % v); return self; }
    Self& operator&=(const Other& v) { auto& self = static_cast<Self&>(*this); assign(self, self & v); return self; }
    Self& operator|=(const Other& v) { auto& self = static_cast<Self&>(*this); assign(self, self | v); return self; }
    Self& operator^=(const Other& v) { auto& self = static_cast<Self&>(*this); assign(self, self ^ v); return self; }
    Self& operator>>=(const Other& v) { auto& self = static_cast<Self&>(*this); assign(self, self >> v); return self; }
    Self& operator<<=(const Other& v) { auto& self = static_cast<Self&>(*this); assign(self, self << v); return self; }
};

template<class Self>
class IntegerBaseWithSelf: public ConstIntegerBaseWithSelf<Self> {
private:
    typedef typename ConstIntegerBaseTrait<Self>::SelfMaxBit SelfMaxBit;
    using ConstIntegerBaseWithSelf<Self>::cast_for_self_op;

    inline static void assign(Self& self, uint32_t v) {
        IntegerBaseTrait<Self>::assign(self, v);
    }

    inline static void assign(Self& self, uint64_t v) {
        IntegerBaseTrait<Self>::assign(self, v);
    }
public:
    Self& operator+=(const Self& v)     { auto& self = static_cast<Self&>(*this); assign(self, self + v); return self;  }
    Self& operator-=(const Self& v)     { auto& self = static_cast<Self&>(*this); assign(self, self - v); return self;  }
    Self& operator*=(const Self& v)     { auto& self = static_cast<Self&>(*this); assign(self, self * v); return self;  }
    Self& operator/=(const Self& v)     { auto& self = static_cast<Self&>(*this); assign(self, self / v); return self;  }
    Self& operator%=(const Self& v)     { auto& self = static_cast<Self&>(*this); assign(self, self % v); return self;  }

    Self& operator++() { auto& self = static_cast<Self&>(*this); assign(self, self + 1); return self; }
    Self& operator--() { auto& self = static_cast<Self&>(*this); assign(self, self - 1); return self; }
    SelfMaxBit operator++(int) { auto& self = static_cast<Self&>(*this); auto tmp = cast_for_self_op(self); assign(self, self + 1); return tmp; }
    SelfMaxBit operator--(int) { auto& self = static_cast<Self&>(*this); auto tmp = cast_for_self_op(self); assign(self, self - 1); return tmp; }

    Self& operator&=(const Self& v)     { auto& self = static_cast<Self&>(*this); assign(self, self & v); return self;  }
    Self& operator|=(const Self& v)     { auto& self = static_cast<Self&>(*this); assign(self, self | v); return self;  }
    Self& operator^=(const Self& v)     { auto& self = static_cast<Self&>(*this); assign(self, self ^ v); return self;  }
    Self& operator>>=(const Self& v)    { auto& self = static_cast<Self&>(*this); assign(self, self >> v); return self; }
    Self& operator<<=(const Self& v)    { auto& self = static_cast<Self&>(*this); assign(self, self << v); return self; }
};


// NB: This is a hack needed to get the lookup rules for the operator memebers
// play nice. The basic issue is that different overloads in different base
// classes will be considered ambigious, so we need to import them all into
// the same base class.

template<typename...>
class IntegerBaseCombiner;

template<typename T, typename... Ts>
class IntegerBaseCombiner<T, Ts...>: public T, public IntegerBaseCombiner<Ts...> {
public:
    using T::operator +=;
    using T::operator -=;
    using T::operator *=;
    using T::operator /=;
    using T::operator %=;
    using T::operator &=;
    using T::operator |=;
    using T::operator ^=;
    using T::operator >>=;
    using T::operator <<=;

    using IntegerBaseCombiner<Ts...>::operator +=;
    using IntegerBaseCombiner<Ts...>::operator +=;
    using IntegerBaseCombiner<Ts...>::operator -=;
    using IntegerBaseCombiner<Ts...>::operator *=;
    using IntegerBaseCombiner<Ts...>::operator /=;
    using IntegerBaseCombiner<Ts...>::operator %=;
    using IntegerBaseCombiner<Ts...>::operator &=;
    using IntegerBaseCombiner<Ts...>::operator |=;
    using IntegerBaseCombiner<Ts...>::operator ^=;
    using IntegerBaseCombiner<Ts...>::operator >>=;
    using IntegerBaseCombiner<Ts...>::operator <<=;
};

template<typename T>
class IntegerBaseCombiner<T>: public T {
public:
    using T::operator +=;
    using T::operator -=;
    using T::operator *=;
    using T::operator /=;
    using T::operator %=;
    using T::operator &=;
    using T::operator |=;
    using T::operator ^=;
    using T::operator >>=;
    using T::operator <<=;
};

template<class Self>
using IntegerBase = IntegerBaseCombiner<
    IntegerBaseWithSelf<Self>,
    IntegerBaseWith32<Self, uint32_t>,
    IntegerBaseWith32<Self, int>,
    IntegerBaseWith64<Self, uint64_t>
>;

}

#endif
