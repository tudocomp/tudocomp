#include <string>
#include <vector>

#include <gtest/gtest.h>

//#include <tudocomp/ds/GenericIntVector.hpp>

#include <tudocomp/util/IntegerBase.hpp>

#include "test_util.hpp"

using namespace tdc;
//using namespace int_vector;

/*TEST(generic_int_vector, ptr) {
    std::vector<uint64_t> data = { 0xff00ffff00001111, 0x0123456789ABCDEF };

    IntPtr a(&data[0], 0, 32);
    IntPtr b(&data[0], 32, 32);
    IntPtr c(&data[1], 0, 32);
    IntPtr d(&data[1], 32, 32);

    ASSERT_EQ(*a, 0x00001111);
    ASSERT_EQ(*b, 0xff00ffff);
    ASSERT_EQ(*c, 0x89ABCDEF);
    ASSERT_EQ(*d, 0x01234567);

    ASSERT_EQ(data, (std::vector<uint64_t> { 0xff00ffff00001111, 0x0123456789ABCDEF }));

    *c = 0;

    ASSERT_EQ(data, (std::vector<uint64_t> { 0xff00ffff00001111, 0x0123456700000000 }));

    a++;

}*/

struct IBTest;

namespace tdc {
    template<>
    struct IntegerBaseTrait<IBTest> {
        typedef uint32_t SelfMaxBit;
        inline static void assign(IBTest& self, uint32_t v);
        inline static void assign(IBTest& self, uint64_t v);
        inline static SelfMaxBit cast_for_self_op(const IBTest& self);
        inline static SelfMaxBit cast_for_32_op(const IBTest& self);
        inline static uint64_t cast_for_64_op(const IBTest& self);
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
inline uint32_t tdc::IntegerBaseTrait<IBTest>::cast_for_self_op(const IBTest& self) { return *self.m_ptr; }
inline uint32_t tdc::IntegerBaseTrait<IBTest>::cast_for_32_op(const IBTest& self)   { return *self.m_ptr; }
inline uint64_t tdc::IntegerBaseTrait<IBTest>::cast_for_64_op(const IBTest& self)   { return *self.m_ptr; }

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
