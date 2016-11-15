#ifndef DYNAMIC_T_HPP
#define DYNAMIC_T_HPP

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

    friend class DyntDispatch;

public:
    dynamic_t(): m_data(0) {}
    dynamic_t(dynamic_t&& i): m_data(i.m_data) {}

    // copying
    inline dynamic_t(const dynamic_t& i): m_data(i.m_data) {}
    inline dynamic_t& operator=(const dynamic_t& b) { m_data = b.m_data; return *this; }

    // 64 bit conversions
    inline dynamic_t(uint64_t i): m_data(i) {}
    inline dynamic_t& operator=(uint64_t data) { m_data = data; return *this; }
    inline operator uint64_t() const { return m_data; }

    // 32 bit conversions
    inline dynamic_t(uint32_t i): m_data(i) {}
    inline dynamic_t& operator=(uint32_t data) { m_data = data; return *this; }
    inline operator uint32_t() const { return m_data; }

    // compatibility with unsuffixed integer literals
    inline dynamic_t(int i): m_data(i) {}
    inline dynamic_t& operator=(int data) { m_data = data; return *this; }
    inline operator int() const { return m_data; }

    // To enable interop with gtest
    inline operator long long int() const { return m_data; }

} __attribute__((packed));

}

#endif /* UINT_T_HPP */
