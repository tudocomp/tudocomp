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

    ASSERT_EQ(w, 0x00001111u);
    ASSERT_EQ(x, 0xff00ffffu);
    ASSERT_EQ(y, 0x89ABCDEFu);
    ASSERT_EQ(z, 0x01234567u);

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

    ASSERT_EQ(w, 0x00001111u);
    ASSERT_EQ(x, 0xff00ffffu);
    ASSERT_EQ(y, 0x89ABCDEFu);
    ASSERT_EQ(z, 0x01234567u);

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

    ASSERT_EQ(w,  0xDEADBEEFu);
    ASSERT_EQ(z,  0x01234567u);
    ASSERT_EQ(*a, 0xDEADBEEFu);
    ASSERT_EQ(*d, 0x01234567u);

    using std::swap;
    swap(w, z);

    ASSERT_EQ(w,  0x01234567u);
    ASSERT_EQ(z,  0xDEADBEEFu);
    ASSERT_EQ(*a, 0x01234567u);
    ASSERT_EQ(*d, 0xDEADBEEFu);

    ASSERT_EQ(data, (std::vector<uint64_t> { 0xCAFEBABE01234567, 0xDEADBEEFBADEAFFE }));

    w = z;
    ASSERT_EQ(w, 0xDEADBEEFu);
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

    ASSERT_EQ(*a, 0x00001111u);
    ASSERT_EQ(*b, 0xff00ffffu);
    ASSERT_EQ(*c, 0x89ABCDEFu);
    ASSERT_EQ(*d, 0x01234567u);

    {
        auto tmp = a;
        a = d;
        d = tmp;

        ASSERT_EQ(*d, 0x00001111u);
        ASSERT_EQ(*a, 0x01234567u);

        tmp = a;
        a = d;
        d = tmp;

        ASSERT_EQ(*a, 0x00001111u);
        ASSERT_EQ(*d, 0x01234567u);
    }

    ASSERT_EQ(a[0], 0x00001111u);
    ASSERT_EQ(a[1], 0xff00ffffu);
    ASSERT_EQ(a[2], 0x89ABCDEFu);
    ASSERT_EQ(a[3], 0x01234567u);

    ASSERT_EQ(data, (std::vector<uint64_t> { 0xff00ffff00001111, 0x0123456789ABCDEF }));

    {
        auto tmp = a++;
        ASSERT_EQ(*a,   0xff00ffffu);
        ASSERT_EQ(*tmp, 0x00001111u);
    }

    {
        auto tmp = ++a;
        ASSERT_EQ(*a,   0x89ABCDEFu);
        ASSERT_EQ(*tmp, 0x89ABCDEFu);
    }

    {
        auto tmp = a += 1;
        ASSERT_EQ(*a,   0x01234567u);
        ASSERT_EQ(*tmp, 0x01234567u);
    }

    {
        auto tmp = a -= 1;
        ASSERT_EQ(*a,   0x89ABCDEFu);
        ASSERT_EQ(*tmp, 0x89ABCDEFu);
    }

    {
        auto tmp = --a;
        ASSERT_EQ(*a,   0xff00ffffu);
        ASSERT_EQ(*tmp, 0xff00ffffu);
    }

    {
        auto tmp = a--;
        ASSERT_EQ(*a,   0x00001111u);
        ASSERT_EQ(*tmp, 0xff00ffffu);
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
    ASSERT_EQ(*c, 0u);
    ASSERT_EQ(data, (std::vector<uint64_t> { 0xff00ffff00001111, 0x0123456700000000 }));
    a[2] = 1;
    ASSERT_EQ(a[2], 1u);
    ASSERT_EQ(data, (std::vector<uint64_t> { 0xff00ffff00001111, 0x0123456700000001 }));

    uint64_t ta = *a;

    *a = ta;
    *a = *b;
    ASSERT_EQ(*a, uint64_t(*b));

    *a = ta;
    *a = uint64_t(*b);
    ASSERT_EQ(*a, uint64_t(*b));

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

template<class T> struct bit_size {
    static const uint64_t size = sizeof(T) * CHAR_BIT;
};
template<size_t N> struct bit_size<uint_t<N>> {
    static const uint64_t size = N;
};

template<class T>
void generic_int_vector_template() {
    using namespace int_vector;
    auto N = bit_size<T>::size;
    std::cout << "tests for " << N << "\n";

    GenericIntVector<T> dflt;
    ASSERT_EQ(dflt.size(), 0);
    ASSERT_EQ(dflt.bit_size(), 0);
    // assert size capa 0

    GenericIntVector<T> fill1(size_t(10));
    ASSERT_EQ(fill1.size(), 10);
    ASSERT_EQ(fill1.bit_size(), 10 * N);
    // assert size 10, values 0, bit_size

    GenericIntVector<T> fill2(size_t(10), T(1));
    ASSERT_EQ(fill2.size(), 10);
    ASSERT_EQ(fill2.bit_size(), 10 * N);
    // assert size 10, values 1, bit_size

    std::vector<T> iter_src_1(10, T(1));
    GenericIntVector<T> range1(iter_src_1.begin(), iter_src_1.end());
    ASSERT_EQ(range1.size(), 10);
    ASSERT_EQ(range1.bit_size(), 10 * N);
    // assert size 10, values 1, bit_size

    std::vector<uint64_t> iter_src_2(10, 1);
    GenericIntVector<T> range2(iter_src_2.begin(), iter_src_2.end());
    ASSERT_EQ(range2.size(), 10);
    ASSERT_EQ(range2.bit_size(), 10 * N);
    // assert size 10, values 1, bit_size

    GenericIntVector<T> copy(fill1);
    ASSERT_EQ(copy.size(), 10);
    ASSERT_EQ(copy.bit_size(), 10 * N);
    // assert of both dflt and copy with divergence

    GenericIntVector<T> move(std::move(copy));
    ASSERT_EQ(move.size(), 10);
    ASSERT_EQ(move.bit_size(), 10 * N);
    // assert of equivalence to prev cpy

    GenericIntVector<T> il1 { T(1), T(2), T(3), T(4) };
    ASSERT_EQ(il1.size(), 4);
    ASSERT_EQ(il1.bit_size(), 4 * N);
    // assert size, bit size and content

    GenericIntVector<T> il2 { 1, 2, 3, 4 };
    ASSERT_EQ(il2.size(), 4);
    ASSERT_EQ(il2.bit_size(), 4 * N);
    // assert size, bit size and content

    GenericIntVector<T> assign_target;
    GenericIntVector<T> assign_src1 { 1, 2, 3, 4 };
    GenericIntVector<T> assign_src2 { 5, 6, 7 };

    assign_target = assign_src1;
    ASSERT_EQ(assign_target.size(), 4);
    ASSERT_EQ(assign_target.bit_size(), 4 * N);
    // assert contents and cpacity

    assign_target = std::move(assign_src2);
    ASSERT_EQ(assign_target.size(), 3);
    ASSERT_EQ(assign_target.bit_size(), 3 * N);
    // assert contents and cpacity

    assign_target = { 8, 9, 10, 11, 12 };
    ASSERT_EQ(assign_target.size(), 5);
    ASSERT_EQ(assign_target.bit_size(), 5 * N);
    // assert contents and cpacity

    GenericIntVector<T> iter_src { 1, 2, 3, 4 };
    std::vector<uint64_t> iter_src_cmp  { T(1), T(2), T(3), T(4) };

    ASSERT_EQ(*iter_src.begin(), *iter_src_cmp.begin());
    ASSERT_EQ(*iter_src.rbegin(), *iter_src_cmp.rbegin());
    ASSERT_EQ(*iter_src.cbegin(), *iter_src_cmp.cbegin());
    ASSERT_EQ(*iter_src.crbegin(), *iter_src_cmp.crbegin());

    std::vector<T> iterd1(iter_src.begin(), iter_src.end());
    ASSERT_EQ(iterd1, (std::vector<T> { 1, 2, 3, 4 }));

    std::vector<T> iterd2(iter_src.rbegin(), iter_src.rend());
    ASSERT_EQ(iterd2, (std::vector<T> { 4, 3, 2, 1 }));

    auto iterd_i = 1;
    for (auto a = iter_src.begin(), b = iter_src.end(); a != b; ++a) {
        *a = iterd_i;
        iterd_i++;
    }
    // asser eq to { 2, 4, 6, 8  }

    for (auto a = iter_src.rbegin(), b = iter_src.rend(); a != b; ++a) {
        *a = iterd_i;
        iterd_i++;
    }
    // asser eq to { 13, 12, 13, 10 }

    const GenericIntVector<T> const_iter_src { 1, 2, 3, 4 };

    std::vector<T> iterd3(const_iter_src.begin(), const_iter_src.end());
    ASSERT_EQ(iterd3, (std::vector<T> { 1, 2, 3, 4 }));

    std::vector<T> iterd4(const_iter_src.rbegin(), const_iter_src.rend());
    ASSERT_EQ(iterd4, (std::vector<T> { 4, 3, 2, 1 }));

    ASSERT_EQ(dflt.max_size(), std::vector<T>().max_size());

    GenericIntVector<T> resize { 1, 2, 3, 4 };
    ASSERT_EQ(resize.size(), 4);
    auto tmp_capa = resize.capacity();
    // assert content 1 2 3 4 size 4 tmp=capacity >= size

    resize.resize(2);
    ASSERT_EQ(resize.size(), 2);
    ASSERT_EQ(tmp_capa, resize.capacity());
    // assert content 1 2 size 2 capacity == tmp

    resize.resize(5, 1);
    ASSERT_EQ(resize.size(), 5);
    // assert content 1 2 1 1 1 size 5 capacity ?

    ASSERT_GE(resize.capacity(), resize.size());

    ASSERT_TRUE(dflt.empty());
    ASSERT_FALSE(fill1.empty());

    GenericIntVector<T> reserve;
    ASSERT_EQ(reserve.capacity(), 0);
    reserve.reserve(10);
    ASSERT_GE(reserve.capacity(), 10);

    reserve.shrink_to_fit();

    GenericIntVector<T> referenced { 1, 2, 3, 4, 5 };
    const GenericIntVector<T> const_referenced { 1, 2, 3, 4, 5 };
    // TODO: Remove _real type_ overloads
    ASSERT_EQ(referenced[0], uint64_t(T(1)));
    ASSERT_EQ(referenced[1], uint64_t(T(2)));
    ASSERT_EQ(referenced[2], uint64_t(T(3)));
    ASSERT_EQ(referenced[3], uint64_t(T(4)));
    ASSERT_EQ(referenced[4], uint64_t(T(5)));

    ASSERT_EQ(const_referenced[0], uint64_t(T(1)));
    ASSERT_EQ(const_referenced[1], uint64_t(T(2)));
    ASSERT_EQ(const_referenced[2], uint64_t(T(3)));
    ASSERT_EQ(const_referenced[3], uint64_t(T(4)));
    ASSERT_EQ(const_referenced[4], uint64_t(T(5)));

    referenced[2] = T(100);
    ASSERT_EQ(referenced[2], uint64_t(T(100)));
    // assert equality with 1, 2, 100, 4, 5

    ASSERT_EQ(referenced.at(0), uint64_t(T(1)));
    ASSERT_EQ(referenced.at(1), uint64_t(T(2)));
    ASSERT_EQ(referenced.at(2), uint64_t(T(100)));
    ASSERT_EQ(referenced.at(3), uint64_t(T(4)));
    ASSERT_EQ(referenced.at(4), uint64_t(T(5)));

    ASSERT_EQ(const_referenced.at(0), uint64_t(T(1)));
    ASSERT_EQ(const_referenced.at(1), uint64_t(T(2)));
    ASSERT_EQ(const_referenced.at(2), uint64_t(T(3)));
    ASSERT_EQ(const_referenced.at(3), uint64_t(T(4)));
    ASSERT_EQ(const_referenced.at(4), uint64_t(T(5)));

    referenced.at(3) = T(99);
    ASSERT_EQ(referenced.at(3), uint64_t(T(99)));
    // assert equality with 1, 2, 100, 4, 5

    {
        bool caught = false;
        try {
            referenced.at(100);
        } catch (const std::out_of_range& e) {
            caught = true;

            if (N % 8 != 0)  {
                std::string what = "Out-of-range access of GenericIntVector: index is 100, size() is 5";
                ASSERT_EQ(e.what(), what);
            }
        }
        ASSERT_TRUE(caught);
    }
    {
        bool caught = false;
        try {
            const_referenced.at(100);
        } catch (const std::out_of_range& e) {
            caught = true;

            if (N % 8 != 0)  {
                std::string what = "Out-of-range access of GenericIntVector: index is 100, size() is 5";
                ASSERT_EQ(e.what(), what);
            }
        }
        ASSERT_TRUE(caught);
    }

    // TODO: Fix casts
    ASSERT_EQ(referenced.front(), uint64_t(T(1)));
    ASSERT_EQ(const_referenced.front(), uint64_t(T(1)));
    ASSERT_EQ(referenced.back(), uint64_t(T(5)));
    ASSERT_EQ(const_referenced.back(), uint64_t(T(5)));

    ASSERT_NE(referenced.data(), nullptr);
    ASSERT_NE(const_referenced.data(), nullptr);

    GenericIntVector<T> assign1;
    assign1.assign(size_t(10), T(1));
    ASSERT_EQ(assign1.size(), 10);
    ASSERT_EQ(assign1.bit_size(), 10 * N);
    // assert size 10, values 1, bit_size

    GenericIntVector<T> assign2;
    assign2.assign(iter_src_1.begin(), iter_src_1.end());
    ASSERT_EQ(assign2.size(), 10);
    ASSERT_EQ(assign2.bit_size(), 10 * N);
    // assert size 10, values 1, bit_size

    GenericIntVector<T> assign3;
    assign3.assign(iter_src_2.begin(), iter_src_2.end());
    ASSERT_EQ(assign3.size(), 10);
    ASSERT_EQ(assign3.bit_size(), 10 * N);
    // assert size 10, values 1, bit_size

    GenericIntVector<T> assign4;
    assign4.assign({ T(1), T(2), T(3), T(4) });
    ASSERT_EQ(assign4.size(), 4);
    ASSERT_EQ(assign4.bit_size(), 4 * N);
    // assert size, bit size and content

    GenericIntVector<T> push_back;
    ASSERT_EQ(push_back.size(), 0);
    ASSERT_EQ(push_back.bit_size(), 0 * N);

    push_back.push_back(T(15));
    ASSERT_EQ(push_back.size(), 1);
    ASSERT_EQ(push_back.bit_size(), 1 * N);
    // ASEERT eq { 15 }

    push_back.push_back(T(9));
    ASSERT_EQ(push_back.size(), 2);
    ASSERT_EQ(push_back.bit_size(), 2 * N);
    // ASEERT eq { 15, 9 }

    push_back.pop_back();
    ASSERT_EQ(push_back.size(), 1);
    ASSERT_EQ(push_back.bit_size(), 1 * N);
    // ASEERT eq { 15 }

    // TODO: Fix uint64_t casts

    GenericIntVector<T> insert1 { 6, 5, 4, 3, 2, 1 };
    auto insert1_r = insert1.insert(insert1.cbegin() + 3, T(9));
    ASSERT_EQ(*insert1_r, uint64_t(T(9)));
    ASSERT_TRUE((insert1.begin() < insert1_r) && (insert1_r < insert1.end()));
    // ASSERT EQ { 6, 5, 4, 9, 3, 2, 1 }

    GenericIntVector<T> insert2 { 6, 5, 4, 3, 2, 1 };
    auto insert2_r = insert2.insert(insert2.cbegin() + 3, 3, T(9));
    ASSERT_EQ(*insert2_r, uint64_t(T(9)));
    ASSERT_TRUE((insert2.begin() < insert2_r) && (insert2_r < insert2.end()));
    // ASSERT EQ { 6, 5, 4, 9, 9, 9, 3, 2, 1 }

    GenericIntVector<T> insert3 { 6, 5, 4, 3, 2, 1 };
    GenericIntVector<T> insert3_src { 9, 8, 7 };
    auto insert3_r = insert3.insert(insert3.cbegin() + 3, insert3_src.begin(), insert3_src.end());
    ASSERT_EQ(*insert3_r, uint64_t(T(9)));
    ASSERT_TRUE((insert3.begin() < insert3_r) && (insert3_r < insert3.end()));
    // ASSERT EQ { 6, 5, 4, 9, 8, 7, 3, 2, 1 }

    GenericIntVector<T> insert4 { 6, 5, 4, 3, 2, 1 };
    auto insert4_v = T(9);
    auto insert4_r = insert4.insert(insert4.cbegin() + 3, std::move(insert4_v));
    ASSERT_EQ(*insert4_r, uint64_t(T(9)));
    ASSERT_TRUE((insert4.begin() < insert4_r) && (insert4_r < insert4.end()));
    // ASSERT EQ { 6, 5, 4, 9, 3, 2, 1 }

    GenericIntVector<T> insert5 { 6, 5, 4, 3, 2, 1 };
    auto insert5_r = insert5.insert(insert5.cbegin() + 3, { 9, 8, 7 });
    ASSERT_EQ(*insert5_r, uint64_t(T(9)));
    ASSERT_TRUE((insert5.begin() < insert5_r) && (insert5_r < insert5.end()));
    // ASSERT EQ { 6, 5, 4, 9, 8, 7, 3, 2, 1 }

    GenericIntVector<T> erase1 { 1, 2, 3, 9, 4, 5, 6 };
    auto erase1_r = erase1.erase(erase1.cbegin() + 3);
    ASSERT_TRUE((erase1.begin() < erase1_r) && (erase1_r < erase1.end()));
    ASSERT_EQ(*erase1_r, uint64_t(T(4)));
    // ASSERT EQ { 1, 2, 3, 4, 5, 6 }

    GenericIntVector<T> erase2 { 1, 2, 3, 9, 4, 5, 6 };
    auto erase2_r = erase2.erase(erase2.cbegin() + 3, erase2.cbegin() + 5);
    ASSERT_TRUE((erase2.begin() < erase2_r) && (erase2_r < erase2.end()));
    ASSERT_EQ(*erase2_r, uint64_t(T(5)));
    // ASSERT EQ { 1, 2, 3, 6 }

    GenericIntVector<T> swap_a { 1, 2, 3 };
    GenericIntVector<T> swap_b { 4, 5, 6 };
    swap_a.swap(swap_b);
    // ASSERT a and b swapped

    GenericIntVector<T> clear { 9, 8, 7 };
    clear.clear();
    ASSERT_EQ(clear.size(), 0);
    ASSERT_EQ(clear.bit_size(), 0);
    // ASSERT eq with empty

    GenericIntVector<T> emplace1 { 1, 2, 3, 4 };
    auto emplace1_r = emplace1.emplace(emplace1.cbegin() + 2, T(125));
    // assert eq 1 2 125 3 4
    ASSERT_TRUE((emplace1.begin() < emplace1_r) && (emplace1_r < emplace1.end()));
    // TODO: Fix
    ASSERT_EQ(*emplace1_r, uint64_t(T(125)));

    GenericIntVector<T> emplace2 { 1, 2, 3, 4 };
    emplace2.emplace_back(125);
    // assert eq 1 2 3 4 125

    // TODO: Add tests for &foo[i], maybe add overload to return IntPtr
}


TEST(generic_int_vector, uint32_t) {
    generic_int_vector_template<uint32_t>();
}

TEST(generic_int_vector, uint_t_32) {
    generic_int_vector_template<uint_t<32>>();
}

TEST(generic_int_vector, uint_t_40) {
    generic_int_vector_template<uint_t<40>>();
}

TEST(generic_int_vector, uint_t_64) {
    generic_int_vector_template<uint_t<64>>();
}

TEST(generic_int_vector, uint_t_24) {
    generic_int_vector_template<uint_t<24>>();
}

TEST(generic_int_vector, uint_t_1) {
    generic_int_vector_template<uint_t<1>>();
}

TEST(generic_int_vector, uint_t_2) {
    generic_int_vector_template<uint_t<2>>();
}

TEST(generic_int_vector, uint_t_7) {
    generic_int_vector_template<uint_t<7>>();
}

template<size_t N>
void generic_int_vector_trait_template() {
    using namespace int_vector;

    typedef typename GenericIntVectorTrait<uint_t<N>>::backing_data T;

    ASSERT_EQ(T::backing2bits(1), 64);
    ASSERT_EQ(T::backing2bits(10), 640);

    ASSERT_EQ(T::bits2backing(0), 0);
    ASSERT_EQ(T::bits2backing(1), 1);
    ASSERT_EQ(T::bits2backing(7), 1);
    ASSERT_EQ(T::bits2backing(63), 1);
    ASSERT_EQ(T::bits2backing(64), 1);
    ASSERT_EQ(T::bits2backing(65), 2);
    ASSERT_EQ(T::bits2backing(127), 2);
    ASSERT_EQ(T::bits2backing(128), 2);
    ASSERT_EQ(T::bits2backing(129), 3);

    ASSERT_EQ(T::elem2bits(1), N);
    ASSERT_EQ(T::elem2bits(10), N * 10);
}

TEST(generic_int_vector_trait_template, N1) {
    generic_int_vector_trait_template<1>();
}

TEST(generic_int_vector_trait_template, N7) {
    generic_int_vector_trait_template<7>();
}

TEST(generic_int_vector_trait_template, N9) {
    generic_int_vector_trait_template<9>();
}
