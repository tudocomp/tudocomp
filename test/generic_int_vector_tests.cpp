#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <tudocomp/ds/GenericIntVector.hpp>
#include <tudocomp/util/IntegerBase.hpp>
#include <tudocomp/ds/uint_t.hpp>

#include "test_util.hpp"

using namespace tdc;

template<class P, class R>
void generic_int_vector_ref_template_const() {
    std::vector<uint64_t> data = { 0xff00ffff00001111, 0x0123456789ABCDEF };

    P a(&data[0], 0, 32);
    P b(&data[0], 32, 32);
    P c(&data[1], 0, 32);
    P d(&data[1], 32, 32);

    R w(a);
    R x(b);
    R y(c);
    R z(d);

    ASSERT_EQ(w, 0x00001111);
    ASSERT_EQ(x, 0xff00ffff);
    ASSERT_EQ(y, 0x89ABCDEF);
    ASSERT_EQ(z, 0x01234567);

    ASSERT_EQ(w + 1, 0x00001112);
    ASSERT_EQ(~x, ~uint64_t(0xff00ffff));

    uint32_t u32 = y;
    uint64_t u64 = y;
    int      i32 = y;

    ASSERT_EQ(u32, 0x89ABCDEF);
    ASSERT_EQ(u64, 0x89ABCDEF);
    ASSERT_EQ(i32, 0x89ABCDEF);

}

template<class P, class R>
void generic_int_vector_ref_template() {
    generic_int_vector_ref_template_const<P, R>();
    std::vector<uint64_t> data = { 0xff00ffff00001111, 0x0123456789ABCDEF };

    P a(&data[0], 0, 32);
    P b(&data[0], 32, 32);
    P c(&data[1], 0, 32);
    P d(&data[1], 32, 32);

    R w(a);
    R x(b);
    R y(c);
    R z(d);

    ASSERT_EQ(w, 0x00001111);
    ASSERT_EQ(x, 0xff00ffff);
    ASSERT_EQ(y, 0x89ABCDEF);
    ASSERT_EQ(z, 0x01234567);

    w = uint32_t(0xDEADBEEF);
    ASSERT_EQ(w, 0xDEADBEEF);

    x = uint64_t(0xCAFEBABE);
    ASSERT_EQ(x, 0xCAFEBABE);

    y = int(0xBADEAFFE);
    ASSERT_EQ(y, 0xBADEAFFE);

    ASSERT_EQ(w, 0xDEADBEEF);
    ASSERT_EQ(x, 0xCAFEBABE);
    ASSERT_EQ(y, 0xBADEAFFE);

    ASSERT_EQ(data, (std::vector<uint64_t> { 0xCAFEBABEDEADBEEF, 0x01234567BADEAFFE }));

    ASSERT_EQ(w, 0xDEADBEEF);
    ASSERT_EQ(z, 0x01234567);
    ASSERT_EQ(*a, 0xDEADBEEF);
    ASSERT_EQ(*d, 0x01234567);

    using std::swap;
    swap(w, z);

    ASSERT_EQ(w, 0x01234567);
    ASSERT_EQ(z, 0xDEADBEEF);
    ASSERT_EQ(*a, 0x01234567);
    ASSERT_EQ(*d, 0xDEADBEEF);

    ASSERT_EQ(data, (std::vector<uint64_t> { 0xCAFEBABE01234567, 0xDEADBEEFBADEAFFE }));
}

TEST(generic_int_vector, int_ref) {
    generic_int_vector_ref_template<int_vector::IntPtr, int_vector::IntRef>();
}

TEST(generic_int_vector, const_int_ref) {
    generic_int_vector_ref_template_const<int_vector::ConstIntPtr, int_vector::ConstIntRef>();
}

template<class P>
void generic_int_vector_ptr_template_const() {
    std::vector<uint64_t> data = { 0xff00ffff00001111, 0x0123456789ABCDEF };

    P a(&data[0], 0, 32);
    P b(&data[0], 32, 32);
    P c(&data[1], 0, 32);
    P d(&data[1], 32, 32);

    ASSERT_EQ(*a, 0x00001111);
    ASSERT_EQ(*b, 0xff00ffff);
    ASSERT_EQ(*c, 0x89ABCDEF);
    ASSERT_EQ(*d, 0x01234567);

    {
        auto tmp = a;
        a = d;
        d = tmp;

        ASSERT_EQ(*d, 0x00001111);
        ASSERT_EQ(*a, 0x01234567);

        tmp = a;
        a = d;
        d = tmp;

        ASSERT_EQ(*a, 0x00001111);
        ASSERT_EQ(*d, 0x01234567);
    }

    ASSERT_EQ(a[0], 0x00001111);
    ASSERT_EQ(a[1], 0xff00ffff);
    ASSERT_EQ(a[2], 0x89ABCDEF);
    ASSERT_EQ(a[3], 0x01234567);

    ASSERT_EQ(data, (std::vector<uint64_t> { 0xff00ffff00001111, 0x0123456789ABCDEF }));

    {
        auto tmp = a++;
        ASSERT_EQ(*a, 0xff00ffff);
        ASSERT_EQ(*tmp, 0x00001111);
    }

    {
        auto tmp = ++a;
        ASSERT_EQ(*a, 0x89ABCDEF);
        ASSERT_EQ(*tmp, 0x89ABCDEF);
    }

    {
        auto tmp = a += 1;
        ASSERT_EQ(*a, 0x01234567);
        ASSERT_EQ(*tmp, 0x01234567);
    }

    {
        auto tmp = a -= 1;
        ASSERT_EQ(*a, 0x89ABCDEF);
        ASSERT_EQ(*tmp, 0x89ABCDEF);
    }

    {
        auto tmp = --a;
        ASSERT_EQ(*a,  0xff00ffff);
        ASSERT_EQ(*tmp, 0xff00ffff);
    }

    {
        auto tmp = a--;
        ASSERT_EQ(*a, 0x00001111);
        ASSERT_EQ(*tmp, 0xff00ffff);
    }

    ASSERT_TRUE(a < b);
    ASSERT_TRUE(b < c);
    ASSERT_TRUE(c < d);

    ASSERT_TRUE(a <= b);
    ASSERT_TRUE(b <= c);
    ASSERT_TRUE(c <= d);

    ASSERT_TRUE(d > c);
    ASSERT_TRUE(c > b);
    ASSERT_TRUE(b > a);

    ASSERT_TRUE(d >= c);
    ASSERT_TRUE(c >= b);
    ASSERT_TRUE(b >= a);

    ASSERT_FALSE(b < a);
    ASSERT_FALSE(c < b);
    ASSERT_FALSE(d < c);

    ASSERT_FALSE(b <= a);
    ASSERT_FALSE(c <= b);
    ASSERT_FALSE(d <= c);

    ASSERT_FALSE(c > d);
    ASSERT_FALSE(b > c);
    ASSERT_FALSE(a > b);

    ASSERT_FALSE(c >= d);
    ASSERT_FALSE(b >= c);
    ASSERT_FALSE(a >= b);

    ASSERT_TRUE(a == a);
    ASSERT_TRUE(b == b);
    ASSERT_TRUE(c == c);
    ASSERT_TRUE(d == d);

    ASSERT_TRUE(a != b);
    ASSERT_TRUE(b != c);
    ASSERT_TRUE(c != d);

    ASSERT_EQ(a + 3, d);
    ASSERT_EQ(d - 3, a);
    ASSERT_EQ(d - a, 3);
    ASSERT_EQ((d+1) - a, 4);
}

template<class P>
void generic_int_vector_ptr_template() {
    generic_int_vector_ptr_template_const<P>();

    std::vector<uint64_t> data = { 0xff00ffff00001111, 0x0123456789ABCDEF };

    P a(&data[0], 0, 32);
    P b(&data[0], 32, 32);
    P c(&data[1], 0, 32);
    P d(&data[1], 32, 32);

    *c = 0;
    ASSERT_EQ(*c, 0);
    ASSERT_EQ(data, (std::vector<uint64_t> { 0xff00ffff00001111, 0x0123456700000000 }));
    a[2] = 1;
    ASSERT_EQ(a[2], 1);
    ASSERT_EQ(data, (std::vector<uint64_t> { 0xff00ffff00001111, 0x0123456700000001 }));

}

TEST(generic_int_vector, int_ptr) {
    using namespace int_vector;
    generic_int_vector_ptr_template<IntPtr>();
}

TEST(generic_int_vector, const_int_ptr) {
    using namespace int_vector;
    generic_int_vector_ptr_template_const<ConstIntPtr>();
}

struct IBTest;

namespace tdc {
    template<>
    struct IntegerBaseTraitConst<IBTest> {
        typedef uint32_t SelfMaxBit;
        inline static SelfMaxBit cast_for_self_op(const IBTest& self);
        inline static SelfMaxBit cast_for_32_op(const IBTest& self);
        inline static uint64_t cast_for_64_op(const IBTest& self);
    };
    template<>
    struct IntegerBaseTrait<IBTest>: IntegerBaseTraitConst<IBTest> {
        inline static void assign(IBTest& self, uint32_t v);
        inline static void assign(IBTest& self, uint64_t v);
    };
}

struct IBTest: public IntegerBase<IBTest> {
    uint8_t* m_ptr;
    inline IBTest(uint8_t* ptr): m_ptr(ptr) {}

    inline operator uint8_t() {
        return *m_ptr;
    }
};

inline void tdc::IntegerBaseTrait<IBTest>::assign(IBTest& self, uint32_t v)         { *self.m_ptr = v;    }
inline void tdc::IntegerBaseTrait<IBTest>::assign(IBTest& self, uint64_t v)         { *self.m_ptr = v;    }
inline uint32_t tdc::IntegerBaseTraitConst<IBTest>::cast_for_self_op(const IBTest& self) { return *self.m_ptr; }
inline uint32_t tdc::IntegerBaseTraitConst<IBTest>::cast_for_32_op(const IBTest& self)   { return *self.m_ptr; }
inline uint64_t tdc::IntegerBaseTraitConst<IBTest>::cast_for_64_op(const IBTest& self)   { return *self.m_ptr; }

TEST(integer_base, basic_binop) {
    uint8_t i = 3;
    auto x = IBTest { &i };
    auto y = x;

    {auto a = x + y;
     ASSERT_EQ(a, 6);}

    {auto a = x - y;
     ASSERT_EQ(a, 0);}

    {auto a = x * y;
     ASSERT_EQ(a, 9);}

    {auto a = x / y;
     ASSERT_EQ(a, 1);}

    {auto a = x % y;
     ASSERT_EQ(a, 0);}

    {auto a = x + 1;
     ASSERT_EQ(a, 4);}

    {auto a = 1 + x;
     ASSERT_EQ(a, 4);}

    {auto a = x + (-1);
     ASSERT_EQ(a, 2);}

    {auto a = (-1) + x;
     ASSERT_EQ(a, 2);}
}

TEST(integer_base, test) {
    uint8_t i = 3;
    auto x = IBTest { &i };
    auto y = x;

    {auto a = x + y;
     ASSERT_EQ(a, 6);}

    {auto a = x - y;
     ASSERT_EQ(a, 0);}

    {auto a = x * y;
     ASSERT_EQ(a, 9);}

    {auto a = x / y;
     ASSERT_EQ(a, 1);}

    {auto a = x % y;
     ASSERT_EQ(a, 0);}

    {auto a = x + 1;
     ASSERT_EQ(a, 4);}

    {auto a = 1 + x;
     ASSERT_EQ(a, 4);}

    {auto a = x + (-1);
     ASSERT_EQ(a, 2);}

    {auto a = (-1) + x;
     ASSERT_EQ(a, 2);}
}

TEST(integer_base, op_asssign) {
    {
        uint8_t i = 3;
        auto x = IBTest { &i };
        x += 2;
        ASSERT_EQ(i, 5);
    }
    {
        uint8_t i = 3;
        auto x = IBTest { &i };
        x -= 2;
        ASSERT_EQ(i, 1);
    }
    {
        uint8_t i = 3;
        auto x = IBTest { &i };
        x *= 2;
        ASSERT_EQ(i, 6);
    }
    {
        uint8_t i = 3;
        auto x = IBTest { &i };
        x /= 3;
        ASSERT_EQ(i, 1);
    }
    {
        uint8_t i = 5;
        auto x = IBTest { &i };
        x %= 2;
        ASSERT_EQ(i, 1);
    }

}

TEST(integer_base, inc_dec) {
    {
        uint8_t i = 3;
        auto x = IBTest { &i };
        auto& y = ++x;
        y += 1;
        ASSERT_EQ(i, 5);
    }
    {
        uint8_t i = 3;
        auto x = IBTest { &i };
        auto y = x++;
        y += 1;
        ASSERT_EQ(i, 4);
    }
    {
        uint8_t i = 3;
        auto x = IBTest { &i };
        auto& y = --x;
        y -= 1;
        ASSERT_EQ(i, 1);
    }
    {
        uint8_t i = 3;
        auto x = IBTest { &i };
        auto y = x--;
        y -= 1;
        ASSERT_EQ(i, 2);
    }

}

TEST(integer_base, bit_ops) {
    {
        uint8_t i = 3;
        auto x = IBTest { &i };

        auto a = x & x;
        ASSERT_EQ(a, 3);
        auto b = x & 1;
        ASSERT_EQ(b, 1);
        auto c = 1 & x;
        ASSERT_EQ(c, 1);
    }
    {
        uint8_t i = 3;
        auto x = IBTest { &i };

        auto a = x | x;
        ASSERT_EQ(a, 3);
        auto b = x | 4;
        ASSERT_EQ(b, 7);
        auto c = 4 | x;
        ASSERT_EQ(c, 7);
    }
    {
        uint8_t i = 3;
        auto x = IBTest { &i };

        auto a = x ^ x;
        ASSERT_EQ(a, 0);
        auto b = x ^ 1;
        ASSERT_EQ(b, 2);
        auto c = 1 ^ x;
        ASSERT_EQ(c, 2);
    }
    {
        uint8_t i = 3;
        auto x = IBTest { &i };

        auto a = ~x;
        ASSERT_EQ(a, ~3);
    }
    {
        uint8_t i = 3;
        auto x = IBTest { &i };

        auto a = x << 1;
        ASSERT_EQ(a, 6);
    }
    {
        uint8_t i = 3;
        auto x = IBTest { &i };

        auto a = x >> 1;
        ASSERT_EQ(a, 1);
    }

}

TEST(integer_base, bit_ops_assign) {
    {
        uint8_t i = 3;
        auto x = IBTest { &i };

        x &= 1;
        ASSERT_EQ(x + 0, 1);
    }
    {
        uint8_t i = 3;
        auto x = IBTest { &i };

        x |= 4;
        ASSERT_EQ(x + 0, 7);
    }
    {
        uint8_t i = 3;
        auto x = IBTest { &i };

        x ^= 1;
        ASSERT_EQ(x + 0, 2);
    }
    {
        uint8_t i = 3;
        auto x = IBTest { &i };

        x <<= 1;
        ASSERT_EQ(x + 0, 6);
    }
    {
        uint8_t i = 3;
        auto x = IBTest { &i };

        x >>= 1;
        ASSERT_EQ(x + 0, 1);
    }


}

template<class T> struct tn { constexpr const static char* str = "unknown"; };
template<>        struct tn<uint32_t> { constexpr const static char* str = "uint32_t"; };
template<>        struct tn<uint64_t> { constexpr const static char* str = "uint64_t"; };
template<class T> std::string type_name(T t) { return tn<T>::str; }

TEST(uint_t, b24) {
    uint_t<24> v;
    v = 0;
    ASSERT_EQ(v, 0u);
    v++;
    ASSERT_EQ(v, 1u);
    v *= 100;
    ASSERT_EQ(v, 100u);
    v = v / 2;
    ASSERT_EQ(v, 50u);
    v = ~0;
    ASSERT_EQ(v, uint64_t((1 << 24) - 1));

    ASSERT_EQ(type_name(v + 1), "uint32_t");

    v = uint8_t(0);
    v = uint16_t(0);
    v = uint32_t(0);
    v = uint64_t(0);
    v = int8_t(0);
    v = int16_t(0);
    v = int32_t(0);
    // v = int64_t(0);
    v = 0;

}

TEST(uint_t, b40) {
    uint_t<40> v;
    v = 0;
    ASSERT_EQ(v, 0u);
    v++;
    ASSERT_EQ(v, 1u);
    v *= 100;
    ASSERT_EQ(v, 100u);
    v = v / 2;
    ASSERT_EQ(v, 50u);
    v = ~0;
    ASSERT_EQ(v, uint64_t((uint64_t(1) << 40) - 1));

    ASSERT_EQ(type_name(v + 1), "uint64_t");

    v = uint8_t(0);
    v = uint16_t(0);
    v = uint32_t(0);
    v = uint64_t(0);
    v = int8_t(0);
    v = int16_t(0);
    v = int32_t(0);
    // v = int64_t(0);
    v = 0;

}
