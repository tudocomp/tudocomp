#pragma once

#include <tudocomp/util/IntegerBase.hpp>

#include <cstdint>

namespace tdc {

class dynamic_t;

struct DyntDispatch {
    typedef uint64_t SelfMaxBit;

    template<class Ref, class V>
    inline static void assign(Ref& self, V v) {
        self.m_data = v;
    };

    template<class Ref, class R>
    inline static R cast_for_op(const Ref& self) {
        return self.m_data;
    }
};

template<>
struct ConstIntegerBaseTrait<dynamic_t> {
    typedef DyntDispatch Dispatch;
};

template<>
struct IntegerBaseTrait<dynamic_t>: ConstIntegerBaseTrait<dynamic_t> {
    typedef DyntDispatch Dispatch;
};

/// Custom integer type for storing values with a runtime-variable number of
/// bits, with an maximum of 64 bits.
///
/// The bit with, however, is not actually stored in an instance of this type,
/// so it behaves the same as a `uint64_t` in practice.
///
/// It exists to support the `GenericIntVector<dynamic_t>` specialization.
class dynamic_t: public IntegerBase<dynamic_t> {
    uint64_t m_data;

    friend struct DyntDispatch;

public:
    dynamic_t(): m_data(0) {}
    dynamic_t(dynamic_t&& i): m_data(i.m_data) {}

    // copying
    inline dynamic_t(const dynamic_t& i): m_data(i.m_data) {}
    inline dynamic_t& operator=(const dynamic_t& b) { m_data = b.m_data; return *this; }

    // conversions for all fundamental char types
    inline dynamic_t(unsigned char i): m_data(i) {}
    inline dynamic_t& operator=(unsigned char data) { m_data = data; return *this; }
    inline operator unsigned char() const { return m_data; }

    inline dynamic_t(signed char i): m_data(i) {}
    inline dynamic_t& operator=(signed char data) { m_data = data; return *this; }
    inline operator signed char() const { return m_data; }

    inline dynamic_t(char i): m_data(i) {}
    inline dynamic_t& operator=(char data) { m_data = data; return *this; }
    inline operator char() const { return m_data; }

    // conversions for all fundamental integer types
    inline dynamic_t(unsigned int i): m_data(i) {}
    inline dynamic_t& operator=(unsigned int data) { m_data = data; return *this; }
    inline operator unsigned int() const { return m_data; }

    inline dynamic_t(unsigned short int i): m_data(i) {}
    inline dynamic_t& operator=(unsigned short int data) { m_data = data; return *this; }
    inline operator unsigned short int() const { return m_data; }

    inline dynamic_t(unsigned long int i): m_data(i) {}
    inline dynamic_t& operator=(unsigned long int data) { m_data = data; return *this; }
    inline operator unsigned long int() const { return m_data; }

    inline dynamic_t(unsigned long long int i): m_data(i) {}
    inline dynamic_t& operator=(unsigned long long int data) { m_data = data; return *this; }
    inline operator unsigned long long int() const { return m_data; }

    inline dynamic_t(int i): m_data(i) {}
    inline dynamic_t& operator=(int data) { m_data = data; return *this; }
    inline operator int() const { return m_data; }

    inline dynamic_t(short int i): m_data(i) {}
    inline dynamic_t& operator=(short int data) { m_data = data; return *this; }
    inline operator short int() const { return m_data; }

    inline dynamic_t(long int i): m_data(i) {}
    inline dynamic_t& operator=(long int data) { m_data = data; return *this; }
    inline operator long int() const { return m_data; }

    inline dynamic_t(long long int i): m_data(i) {}
    inline dynamic_t& operator=(long long int data) { m_data = data; return *this; }
    inline operator long long int() const { return m_data; }
} __attribute__((packed));

inline std::ostream& operator<<(std::ostream& os, const dynamic_t& v) {
    return os << uint64_t(v);
}

}

