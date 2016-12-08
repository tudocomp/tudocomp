#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <tudocomp/ds/IntVector.hpp>
#include <tudocomp/util/IntegerBase.hpp>
#include <tudocomp/ds/uint_t.hpp>

#include "test/util.hpp"

using namespace tdc;

template<class P, class R, class T>
void generic_int_vector_ref_template_const() {
    using Data = typename int_vector::IntPtrTrait<P>::Data;

    std::vector<uint64_t> data = { 0xff00ffff00001111, 0x0123456789ABCDEF };

    P a(Data(&data[0], 0, 32));
    P b(Data(&data[0], 32, 32));
    P c(Data(&data[1], 0, 32));
    P d(Data(&data[1], 32, 32));

    R w(a);
    R x(b);
    R y(c);
    R z(d);

    ASSERT_EQ(w, 0x00001111u);
    ASSERT_EQ(x, 0xff00ffffu);
    ASSERT_EQ(y, 0x89ABCDEFu);
    ASSERT_EQ(z, 0x01234567u);

    ASSERT_EQ(w + 1, 0x00001112);
    ASSERT_EQ(~x, ~T(0xff00ffff));

    uint32_t u32 = T(y);
    uint64_t u64 = T(y);
    int      i32 = T(y);

    ASSERT_EQ(u32, 0x89ABCDEF);
    ASSERT_EQ(u64, 0x89ABCDEF);
    ASSERT_EQ(i32, 0x89ABCDEF);

}

template<class P, class R, class T>
void generic_int_vector_ref_template() {
    using Data = typename int_vector::IntPtrTrait<P>::Data;
    generic_int_vector_ref_template_const<P, R, T>();
    std::vector<uint64_t> data = { 0xff00ffff00001111, 0x0123456789ABCDEF };

    P a(Data(&data[0], 0, 32));
    P b(Data(&data[0], 32, 32));
    P c(Data(&data[1], 0, 32));
    P d(Data(&data[1], 32, 32));

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
    generic_int_vector_ref_template<int_vector::IntPtr<uint_t<32>>, int_vector::IntRef<uint_t<32>>, uint_t<32>>();
}

TEST(generic_int_vector, const_int_ref) {
    generic_int_vector_ref_template_const<int_vector::ConstIntPtr<uint_t<32>>, int_vector::ConstIntRef<uint_t<32>>, uint_t<32>>();
}

template<class P>
void generic_int_vector_ptr_template_const() {
    using Data = typename int_vector::IntPtrTrait<P>::Data;
    std::vector<uint64_t> data = { 0xff00ffff00001111, 0x0123456789ABCDEF };

    P();

    P a(Data(&data[0], 0, 32));
    P b(Data(&data[0], 32, 32));
    P c(Data(&data[1], 0, 32));
    P d(Data(&data[1], 32, 32));

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

template<class P, class T>
void generic_int_vector_ptr_template() {
    using Data = typename int_vector::IntPtrTrait<P>::Data;
    generic_int_vector_ptr_template_const<P>();

    std::vector<uint64_t> data = { 0xff00ffff00001111, 0x0123456789ABCDEF };

    P a(Data(&data[0], 0, 32));
    P b(Data(&data[0], 32, 32));
    P c(Data(&data[1], 0, 32));
    P d(Data(&data[1], 32, 32));

    *c = 0;
    ASSERT_EQ(*c, 0u);
    ASSERT_EQ(data, (std::vector<uint64_t> { 0xff00ffff00001111, 0x0123456700000000 }));
    a[2] = 1;
    ASSERT_EQ(a[2], 1u);
    ASSERT_EQ(data, (std::vector<uint64_t> { 0xff00ffff00001111, 0x0123456700000001 }));

    uint64_t ta = T(*a);

    *a = ta;
    *a = *b;
    ASSERT_EQ(*a, T(*b));

    *a = ta;
    *a = T(*b);
    ASSERT_EQ(*a, T(*b));

}

TEST(generic_int_vector, int_ptr) {
    generic_int_vector_ptr_template<IntPtr<uint_t<32>>, uint_t<32>>();
}

TEST(generic_int_vector, const_int_ptr) {
    generic_int_vector_ptr_template_const<ConstIntPtr<uint_t<32>>>();
}

struct IBTest;

namespace tdc {
    struct IBTestDispatch {
        typedef uint32_t SelfMaxBit;

        template<class Ref, class V>
        inline static void assign(Ref& self, V v) {
            *self.m_ptr = v;
        };

        template<class Ref, class R>
        inline static R cast_for_op(const Ref& self) {
            return *self.m_ptr;
        }
    };

    template<>
    struct ConstIntegerBaseTrait<IBTest> {
        typedef IBTestDispatch Dispatch;
    };

    template<>
    struct IntegerBaseTrait<IBTest>: ConstIntegerBaseTrait<IBTest> {
        typedef IBTestDispatch Dispatch;
    };
}

struct IBTest: public IntegerBase<IBTest> {
    uint8_t* m_ptr;
    inline IBTest(uint8_t* ptr): m_ptr(ptr) {}

    inline operator uint8_t() {
        return *m_ptr;
    }
};

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

TEST(uint_t, multi_inherit) {
    uint_t<7> x = 0;
    x = x + uint32_t(1);
    x = x + uint64_t(1);
    x = x + int(1);
    ASSERT_EQ(int(x), 3);
}

template<class T> struct bit_size {
    static const uint64_t size = sizeof(T) * CHAR_BIT;
    static const bool dynamic = false;
};
template<size_t N> struct bit_size<uint_t<N>> {
    static const uint64_t size = N;
    static const bool dynamic = false;
};
template<> struct bit_size<dynamic_t> {
    static const uint64_t size = 64;
    static const bool dynamic = true;
};

template<class T>
void generic_int_vector_template() {
    namespace iv = int_vector;
    auto N = bit_size<T>::size;
    using V = typename IntVector<T>::value_type;

    std::cout << "tests for " << N << "\n";

    if (N % 8 == 0 && !bit_size<T>::dynamic) {
        ASSERT_EQ(IntVector<T>::element_storage_mode(), iv::ElementStorageMode::Direct);
    } else {
        ASSERT_EQ(IntVector<T>::element_storage_mode(), iv::ElementStorageMode::BitPacked);
    }

    ASSERT_TRUE ((IntVector<T> { 1, 2, 3 }) == (IntVector<T> { 1, 2, 3 }));
    ASSERT_TRUE ((IntVector<T> {         }) == (IntVector<T> {         }));
    ASSERT_TRUE ((IntVector<T> { 9       }) == (IntVector<T> { 9       }));
    ASSERT_FALSE((IntVector<T> { 1, 2, 3 }) == (IntVector<T> { 1, 2, 4 }));
    ASSERT_FALSE((IntVector<T> { 1, 2, 3 }) == (IntVector<T> { 1, 2    }));

    ASSERT_TRUE ((IntVector<T> { 1, 2, 3 }) != (IntVector<T> { 1, 2, 4 }));
    ASSERT_TRUE ((IntVector<T> { 1, 2, 3 }) != (IntVector<T> { 1, 2    }));
    ASSERT_TRUE ((IntVector<T> {         }) != (IntVector<T> { 1, 2    }));
    ASSERT_FALSE((IntVector<T> { 1, 2, 3 }) != (IntVector<T> { 1, 2, 3 }));

    ASSERT_TRUE ((IntVector<T> { 0, 2, 4 }) < (IntVector<T> { 0, 2, 5 }));
    ASSERT_TRUE ((IntVector<T> { 1, 2    }) < (IntVector<T> { 1, 2, 3 }));
    ASSERT_TRUE ((IntVector<T> { 0, 1, 3 }) < (IntVector<T> { 3, 0, 0 }));
    ASSERT_TRUE ((IntVector<T> {         }) < (IntVector<T> { 1       }));
    ASSERT_FALSE((IntVector<T> { 0, 1, 2 }) < (IntVector<T> { 0, 1, 2 }));
    ASSERT_FALSE((IntVector<T> { 0, 2, 5 }) < (IntVector<T> { 0, 2, 4 }));
    ASSERT_FALSE((IntVector<T> { 1, 2, 3 }) < (IntVector<T> { 1, 2    }));
    ASSERT_FALSE((IntVector<T> { 3, 0, 0 }) < (IntVector<T> { 0, 1, 3 }));
    ASSERT_FALSE((IntVector<T> { 1,      }) < (IntVector<T> {         }));

    ASSERT_TRUE ((IntVector<T> { 0, 2, 5 }) > (IntVector<T> { 0, 2, 4 }));
    ASSERT_TRUE ((IntVector<T> { 1, 2, 3 }) > (IntVector<T> { 1, 2    }));
    ASSERT_TRUE ((IntVector<T> { 3, 0, 0 }) > (IntVector<T> { 0, 1, 3 }));
    ASSERT_TRUE ((IntVector<T> { 1,      }) > (IntVector<T> {         }));
    ASSERT_FALSE((IntVector<T> { 0, 1, 2 }) > (IntVector<T> { 0, 1, 2 }));
    ASSERT_FALSE((IntVector<T> { 0, 2, 4 }) > (IntVector<T> { 0, 2, 5 }));
    ASSERT_FALSE((IntVector<T> { 1, 2    }) > (IntVector<T> { 1, 2, 3 }));
    ASSERT_FALSE((IntVector<T> { 0, 1, 3 }) > (IntVector<T> { 3, 0, 0 }));
    ASSERT_FALSE((IntVector<T> {         }) > (IntVector<T> { 1       }));

    ASSERT_TRUE ((IntVector<T> { 0, 2, 4 }) <= (IntVector<T> { 0, 2, 5 }));
    ASSERT_TRUE ((IntVector<T> { 1, 2    }) <= (IntVector<T> { 1, 2, 3 }));
    ASSERT_TRUE ((IntVector<T> { 0, 1, 3 }) <= (IntVector<T> { 3, 0, 0 }));
    ASSERT_TRUE ((IntVector<T> {         }) <= (IntVector<T> { 1       }));
    ASSERT_TRUE ((IntVector<T> { 0, 1, 2 }) <= (IntVector<T> { 0, 1, 2 }));
    ASSERT_FALSE((IntVector<T> { 0, 2, 5 }) <= (IntVector<T> { 0, 2, 4 }));
    ASSERT_FALSE((IntVector<T> { 1, 2, 3 }) <= (IntVector<T> { 1, 2    }));
    ASSERT_FALSE((IntVector<T> { 3, 0, 0 }) <= (IntVector<T> { 0, 1, 3 }));
    ASSERT_FALSE((IntVector<T> { 1,      }) <= (IntVector<T> {         }));

    ASSERT_TRUE ((IntVector<T> { 0, 2, 5 }) >= (IntVector<T> { 0, 2, 4 }));
    ASSERT_TRUE ((IntVector<T> { 1, 2, 3 }) >= (IntVector<T> { 1, 2    }));
    ASSERT_TRUE ((IntVector<T> { 3, 0, 0 }) >= (IntVector<T> { 0, 1, 3 }));
    ASSERT_TRUE ((IntVector<T> { 1,      }) >= (IntVector<T> {         }));
    ASSERT_TRUE ((IntVector<T> { 0, 1, 2 }) >= (IntVector<T> { 0, 1, 2 }));
    ASSERT_FALSE((IntVector<T> { 0, 2, 4 }) >= (IntVector<T> { 0, 2, 5 }));
    ASSERT_FALSE((IntVector<T> { 1, 2    }) >= (IntVector<T> { 1, 2, 3 }));
    ASSERT_FALSE((IntVector<T> { 0, 1, 3 }) >= (IntVector<T> { 3, 0, 0 }));
    ASSERT_FALSE((IntVector<T> {         }) >= (IntVector<T> { 1       }));

    IntVector<T> dflt;
    ASSERT_EQ(dflt.size(), 0);
    ASSERT_EQ(dflt.bit_size(), 0);
    ASSERT_EQ(dflt, IntVector<T> {});

    IntVector<T> fill1(size_t(10));
    ASSERT_EQ(fill1.size(), 10);
    ASSERT_EQ(fill1.bit_size(), 10 * N);
    ASSERT_EQ(fill1, (IntVector<T> { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }));

    IntVector<T> fill2(size_t(10), V(1));
    ASSERT_EQ(fill2.size(), 10);
    ASSERT_EQ(fill2.bit_size(), 10 * N);
    ASSERT_EQ(fill2, (IntVector<T> { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }));

    std::vector<V> iter_src_1(10, V(1));
    IntVector<T> range1(iter_src_1.begin(), iter_src_1.end());
    ASSERT_EQ(range1.size(), 10);
    ASSERT_EQ(range1.bit_size(), 10 * N);
    ASSERT_EQ(range1, (IntVector<T> { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }));

    std::vector<uint64_t> iter_src_2(10, 1);
    IntVector<T> range2(iter_src_2.begin(), iter_src_2.end());
    ASSERT_EQ(range2.size(), 10);
    ASSERT_EQ(range2.bit_size(), 10 * N);
    ASSERT_EQ(range2, (IntVector<T> { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }));

    IntVector<T> copy(fill1);
    ASSERT_EQ(copy.size(), 10);
    ASSERT_EQ(copy.bit_size(), 10 * N);
    ASSERT_EQ(copy, (IntVector<T> { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }));
    ASSERT_EQ(copy, fill1);

    IntVector<T> move(std::move(copy));
    ASSERT_EQ(move.size(), 10);
    ASSERT_EQ(move.bit_size(), 10 * N);
    ASSERT_EQ(move, (IntVector<T> { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }));
    ASSERT_EQ(move, fill1);

    IntVector<T> il1 { V(1), V(2), V(3), V(4) };
    ASSERT_EQ(il1.size(), 4);
    ASSERT_EQ(il1.bit_size(), 4 * N);
    ASSERT_EQ(il1, (IntVector<T> { 1, 2, 3, 4 }));

    IntVector<T> il2 { 1, 2, 3, 4 };
    ASSERT_EQ(il2.size(), 4);
    ASSERT_EQ(il2.bit_size(), 4 * N);
    ASSERT_EQ(il2, (IntVector<T> { 1, 2, 3, 4 }));

    IntVector<T> assign_target;
    IntVector<T> assign_src1 { 1, 2, 3, 4 };
    IntVector<T> assign_src2 { 5, 6, 7 };

    assign_target = assign_src1;
    ASSERT_EQ(assign_target.size(), 4);
    ASSERT_EQ(assign_target.bit_size(), 4 * N);
    ASSERT_EQ(assign_target, (IntVector<T> { 1, 2, 3, 4 }));

    assign_target = std::move(assign_src2);
    ASSERT_EQ(assign_target.size(), 3);
    ASSERT_EQ(assign_target.bit_size(), 3 * N);
    ASSERT_EQ(assign_target, (IntVector<T> { 5, 6, 7 }));

    assign_target = { 8, 9, 10, 11, 12 };
    ASSERT_EQ(assign_target.size(), 5);
    ASSERT_EQ(assign_target.bit_size(), 5 * N);
    ASSERT_EQ(assign_target, (IntVector<T> { 8, 9, 10, 11, 12 }));

    IntVector<T> iter_src { 1, 2, 3, 4 };
    std::vector<uint64_t> iter_src_cmp  { V(1), V(2), V(3), V(4) };

    ASSERT_EQ(*iter_src.begin(), *iter_src_cmp.begin());
    ASSERT_EQ(*iter_src.rbegin(), *iter_src_cmp.rbegin());
    ASSERT_EQ(*iter_src.cbegin(), *iter_src_cmp.cbegin());
    ASSERT_EQ(*iter_src.crbegin(), *iter_src_cmp.crbegin());

    std::vector<V> iterd1(iter_src.begin(), iter_src.end());
    ASSERT_EQ(iterd1, (std::vector<V> { 1, 2, 3, 4 }));

    std::vector<V> iterd2(iter_src.rbegin(), iter_src.rend());
    ASSERT_EQ(iterd2, (std::vector<V> { 4, 3, 2, 1 }));

    auto iterd_i = 1;
    for (auto a = iter_src.begin(), b = iter_src.end(); a != b; ++a) {
        *a += iterd_i;
        iterd_i++;
    }
    ASSERT_EQ(iter_src, (IntVector<T> { 2, 4, 6, 8 }));

    for (auto a = iter_src.rbegin(), b = iter_src.rend(); a != b; ++a) {
        *a += iterd_i;
        iterd_i++;
    }
    ASSERT_EQ(iter_src, (IntVector<T> { 10, 11, 12, 13 }));

    const IntVector<T> const_iter_src { 1, 2, 3, 4 };

    std::vector<V> iterd3(const_iter_src.begin(), const_iter_src.end());
    ASSERT_EQ(iterd3, (std::vector<V> { 1, 2, 3, 4 }));

    std::vector<V> iterd4(const_iter_src.rbegin(), const_iter_src.rend());
    ASSERT_EQ(iterd4, (std::vector<V> { 4, 3, 2, 1 }));

    ASSERT_GE(dflt.max_size(), std::vector<uint64_t>().max_size());

    IntVector<T> resize { 1, 2, 3, 4 };
    ASSERT_EQ(resize.size(), 4);
    auto tmp_capa = resize.capacity();
    ASSERT_EQ(resize, (IntVector<T> { 1, 2, 3, 4 }));
    ASSERT_GE(resize.capacity(), resize.size());
    ASSERT_EQ(resize.capacity() * N, resize.bit_capacity());

    resize.resize(2);
    ASSERT_EQ(resize.size(), 2);
    ASSERT_EQ(tmp_capa, resize.capacity());
    ASSERT_EQ(resize.capacity() * N, resize.bit_capacity());
    ASSERT_EQ(resize, (IntVector<T> { 1, 2 }));

    resize.resize(5, 1);
    ASSERT_EQ(resize.size(), 5);
    ASSERT_EQ(resize, (IntVector<T> { 1, 2, 1, 1, 1 }));
    ASSERT_GE(resize.capacity(), resize.size());
    ASSERT_EQ(resize.capacity() * N, resize.bit_capacity());

    ASSERT_TRUE(dflt.empty());
    ASSERT_FALSE(fill1.empty());

    IntVector<T> reserve;
    ASSERT_EQ(reserve.capacity(), 0);
    ASSERT_EQ(resize.capacity() * N, resize.bit_capacity());
    reserve.reserve(10);
    ASSERT_GE(reserve.capacity(), 10);
    ASSERT_EQ(resize.capacity() * N, resize.bit_capacity());

    reserve.shrink_to_fit();

    IntVector<T> referenced { 1, 2, 3, 4, 5 };
    const IntVector<T> const_referenced { 1, 2, 3, 4, 5 };
    ASSERT_EQ(referenced[0], (T(1)));
    ASSERT_EQ(referenced[1], (T(2)));
    ASSERT_EQ(referenced[2], (T(3)));
    ASSERT_EQ(referenced[3], (T(4)));
    ASSERT_EQ(referenced[4], (T(5)));

    ASSERT_EQ(const_referenced[0], (T(1)));
    ASSERT_EQ(const_referenced[1], (T(2)));
    ASSERT_EQ(const_referenced[2], (T(3)));
    ASSERT_EQ(const_referenced[3], (T(4)));
    ASSERT_EQ(const_referenced[4], (T(5)));

    referenced[2] = V(100);
    ASSERT_EQ(referenced[2], (V(100)));
    ASSERT_EQ(referenced, (IntVector<T> { 1, 2, 100, 4, 5 }));

    ASSERT_EQ(referenced.at(0), (V(1)));
    ASSERT_EQ(referenced.at(1), (V(2)));
    ASSERT_EQ(referenced.at(2), (V(100)));
    ASSERT_EQ(referenced.at(3), (V(4)));
    ASSERT_EQ(referenced.at(4), (V(5)));

    ASSERT_EQ(const_referenced.at(0), (V(1)));
    ASSERT_EQ(const_referenced.at(1), (V(2)));
    ASSERT_EQ(const_referenced.at(2), (V(3)));
    ASSERT_EQ(const_referenced.at(3), (V(4)));
    ASSERT_EQ(const_referenced.at(4), (V(5)));

    referenced.at(3) = V(99);
    ASSERT_EQ(referenced.at(3), (V(99)));
    ASSERT_EQ(referenced, (IntVector<T> { 1, 2, 100, 99, 5 }));

    {
        bool caught = false;
        try {
            referenced.at(100);
        } catch (const std::out_of_range& e) {
            caught = true;

            if (N % 8 != 0)  {
                std::string what = "Out-of-range access of IntVector: index is 100, size() is 5";
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
                std::string what = "Out-of-range access of IntVector: index is 100, size() is 5";
                ASSERT_EQ(e.what(), what);
            }
        }
        ASSERT_TRUE(caught);
    }

    ASSERT_EQ(referenced.front(), (T(1)));
    ASSERT_EQ(const_referenced.front(), (T(1)));
    ASSERT_EQ(referenced.back(), (T(5)));
    ASSERT_EQ(const_referenced.back(), (T(5)));

    ASSERT_NE(referenced.data(), nullptr);
    ASSERT_NE(const_referenced.data(), nullptr);

    IntVector<T> assign1;
    assign1.assign(size_t(10), T(1));
    ASSERT_EQ(assign1.size(), 10);
    ASSERT_EQ(assign1.bit_size(), 10 * N);
    ASSERT_EQ(assign1, (IntVector<T> { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }));

    IntVector<T> assign2;
    assign2.assign(iter_src_1.begin(), iter_src_1.end());
    ASSERT_EQ(assign2.size(), 10);
    ASSERT_EQ(assign2.bit_size(), 10 * N);
    ASSERT_EQ(assign2, (IntVector<T> { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }));

    IntVector<T> assign3;
    assign3.assign(iter_src_2.begin(), iter_src_2.end());
    ASSERT_EQ(assign3.size(), 10);
    ASSERT_EQ(assign3.bit_size(), 10 * N);
    ASSERT_EQ(assign3, (IntVector<T> { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }));

    IntVector<T> assign4;
    assign4.assign({ T(1), T(2), T(3), T(4) });
    ASSERT_EQ(assign4.size(), 4);
    ASSERT_EQ(assign4.bit_size(), 4 * N);
    ASSERT_EQ(assign4, (IntVector<T> { 1, 2, 3, 4 }));

    IntVector<T> push_back;
    ASSERT_EQ(push_back.size(), 0);
    ASSERT_EQ(push_back.bit_size(), 0 * N);
    ASSERT_EQ(push_back, (IntVector<T> { }));

    push_back.push_back(T(15));
    ASSERT_EQ(push_back.size(), 1);
    ASSERT_EQ(push_back.bit_size(), 1 * N);
    ASSERT_EQ(push_back, (IntVector<T> { 15 }));

    push_back.push_back(T(9));
    ASSERT_EQ(push_back.size(), 2);
    ASSERT_EQ(push_back.bit_size(), 2 * N);
    ASSERT_EQ(push_back, (IntVector<T> { 15, 9 }));

    push_back.pop_back();
    ASSERT_EQ(push_back.size(), 1);
    ASSERT_EQ(push_back.bit_size(), 1 * N);
    ASSERT_EQ(push_back, (IntVector<T> { 15 }));

    IntVector<T> insert1 { 6, 5, 4, 3, 2, 1 };
    auto insert1_r = insert1.insert(insert1.cbegin() + 3, T(9));
    ASSERT_EQ(*insert1_r, (T(9)));
    ASSERT_TRUE((insert1.begin() < insert1_r) && (insert1_r < insert1.end()));
    ASSERT_EQ(insert1, (IntVector<T> { 6, 5, 4, 9, 3, 2, 1 }));

    IntVector<T> insert2 { 6, 5, 4, 3, 2, 1 };
    auto insert2_r = insert2.insert(insert2.cbegin() + 3, 3, T(9));
    ASSERT_EQ(*insert2_r, (T(9)));
    ASSERT_TRUE((insert2.begin() < insert2_r) && (insert2_r < insert2.end()));
    ASSERT_EQ(insert2, (IntVector<T> { 6, 5, 4, 9, 9, 9, 3, 2, 1 }));

    IntVector<T> insert3 { 6, 5, 4, 3, 2, 1 };
    IntVector<T> insert3_src { 9, 8, 7 };
    auto insert3_r = insert3.insert(insert3.cbegin() + 3, insert3_src.begin(), insert3_src.end());
    ASSERT_EQ(*insert3_r, (T(9)));
    ASSERT_TRUE((insert3.begin() < insert3_r) && (insert3_r < insert3.end()));
    ASSERT_EQ(insert3, (IntVector<T> { 6, 5, 4, 9, 8, 7, 3, 2, 1 }));

    IntVector<T> insert4 { 6, 5, 4, 3, 2, 1 };
    auto insert4_v = T(9);
    auto insert4_r = insert4.insert(insert4.cbegin() + 3, std::move(insert4_v));
    ASSERT_EQ(*insert4_r, (T(9)));
    ASSERT_TRUE((insert4.begin() < insert4_r) && (insert4_r < insert4.end()));
    ASSERT_EQ(insert4, (IntVector<T> { 6, 5, 4, 9, 3, 2, 1 }));

    IntVector<T> insert5 { 6, 5, 4, 3, 2, 1 };
    auto insert5_r = insert5.insert(insert5.cbegin() + 3, { 9, 8, 7 });
    ASSERT_EQ(*insert5_r, (T(9)));
    ASSERT_TRUE((insert5.begin() < insert5_r) && (insert5_r < insert5.end()));
    ASSERT_EQ(insert5, (IntVector<T> { 6, 5, 4, 9, 8, 7, 3, 2, 1 }));

    IntVector<T> erase1 { 1, 2, 3, 9, 4, 5, 6 };
    auto erase1_r = erase1.erase(erase1.cbegin() + 3);
    ASSERT_TRUE((erase1.begin() < erase1_r) && (erase1_r < erase1.end()));
    ASSERT_EQ(*erase1_r, (T(4)));
    ASSERT_EQ(erase1, (IntVector<T> { 1, 2, 3, 4, 5, 6 }));

    IntVector<T> erase2 { 1, 2, 3, 9, 4, 5, 6 };
    auto erase2_r = erase2.erase(erase2.cbegin() + 3, erase2.cbegin() + 5);
    ASSERT_TRUE((erase2.begin() < erase2_r) && (erase2_r < erase2.end()));
    ASSERT_EQ(*erase2_r, (T(5)));
    ASSERT_EQ(erase2, (IntVector<T> { 1, 2, 3, 5, 6 }));

    IntVector<T> swap_a { 1, 2, 3 };
    IntVector<T> swap_b { 4, 5, 6 };
    swap_a.swap(swap_b);
    ASSERT_EQ(swap_a, (IntVector<T> { 4, 5, 6 }));
    ASSERT_EQ(swap_b, (IntVector<T> { 1, 2, 3 }));

    IntVector<T> clear { 9, 8, 7 };
    clear.clear();
    ASSERT_EQ(clear.size(), 0);
    ASSERT_EQ(clear.bit_size(), 0);
    ASSERT_EQ(clear, (IntVector<T> { }));

    IntVector<T> emplace1 { 1, 2, 3, 4 };
    auto emplace1_r = emplace1.emplace(emplace1.cbegin() + 2, T(125));
    ASSERT_EQ(emplace1, (IntVector<T> { 1, 2, 125, 3, 4 }));
    ASSERT_TRUE((emplace1.begin() < emplace1_r) && (emplace1_r < emplace1.end()));

    ASSERT_EQ(*emplace1_r, (T(125)));

    IntVector<T> emplace2 { 1, 2, 3, 4 };
    emplace2.emplace_back(125);
    ASSERT_EQ(emplace2, (IntVector<T> { 1, 2, 3, 4, 125 }));

    IntVector<T> ref_ptr { 1, 2, 3 };
    auto ptr = &ref_ptr[1];

    ASSERT_EQ(*ptr, (T(2)));

    using reference_t = typename IntVector<T>::reference;
    using const_reference_t = typename IntVector<T>::const_reference;
    using pointer_t = typename IntVector<T>::pointer;
    using const_pointer_t = typename IntVector<T>::const_pointer;

    IntVector<T> const_conv_1 { 1 };
    reference_t const_conv_1_ref = const_conv_1[0];
    pointer_t const_conv_1_ptr = &const_conv_1[0];
    const const_reference_t const_conv_1_const_ref = const_conv_1_ref;
    const const_pointer_t const_conv_1_const_ptr = const_conv_1_ptr;
    const const_reference_t const_conv_2_const_ref = const_conv_1[0];
    const const_pointer_t const_conv_2_const_ptr = &const_conv_1[0];

    ASSERT_EQ(const_conv_1_const_ref, const_conv_2_const_ref);
    ASSERT_EQ(const_conv_1_const_ptr, const_conv_2_const_ptr);
}

// TODO: Test constness of operations

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

TEST(generic_int_vector, uint_t_9) {
    generic_int_vector_template<uint_t<9>>();
}

TEST(generic_int_vector, uint_t_7) {
    generic_int_vector_template<uint_t<7>>();
}

TEST(generic_int_vector, uint_t_2) {
    generic_int_vector_template<uint_t<2>>();
}

TEST(generic_int_vector, uint_t_1) {
    generic_int_vector_template<uint_t<1>>();
}

TEST(generic_int_vector, dynamic_t) {
    generic_int_vector_template<dynamic_t>();
}

TEST(generic_int_vector, dynamic_t_extra) {
    namespace iv = int_vector;

    IntVector<dynamic_t> a(3, 3, 1);
    IntVector<dynamic_t> b(3, 3, 2);
    IntVector<dynamic_t> c(size_t(3), dynamic_t(4));

    {
        ASSERT_EQ(a.width(), 1);
        ASSERT_EQ(b.width(), 2);
        ASSERT_EQ(c.width(), 64);

        std::vector<dynamic_t> av(a.begin(), a.end());
        std::vector<dynamic_t> bv(b.begin(), b.end());
        std::vector<dynamic_t> cv(c.begin(), c.end());

        ASSERT_EQ(av, (std::vector<dynamic_t> { 1, 1, 1 }));
        ASSERT_EQ(bv, (std::vector<dynamic_t> { 3, 3, 3 }));
        ASSERT_EQ(cv, (std::vector<dynamic_t> { 4, 4, 4 }));
    }

    a.swap(b);

    {
        ASSERT_EQ(a.width(), 2);
        ASSERT_EQ(b.width(), 1);
        ASSERT_EQ(c.width(), 64);

        std::vector<dynamic_t> av(a.begin(), a.end());
        std::vector<dynamic_t> bv(b.begin(), b.end());
        std::vector<dynamic_t> cv(c.begin(), c.end());

        ASSERT_EQ(av, (std::vector<dynamic_t> { 3, 3, 3 }));
        ASSERT_EQ(bv, (std::vector<dynamic_t> { 1, 1, 1 }));
        ASSERT_EQ(cv, (std::vector<dynamic_t> { 4, 4, 4 }));
    }

    b = c;

    {
        ASSERT_EQ(a.width(), 2);
        ASSERT_EQ(b.width(), 64);
        ASSERT_EQ(c.width(), 64);

        std::vector<dynamic_t> av(a.begin(), a.end());
        std::vector<dynamic_t> bv(b.begin(), b.end());
        std::vector<dynamic_t> cv(c.begin(), c.end());

        ASSERT_EQ(av, (std::vector<dynamic_t> { 3, 3, 3 }));
        ASSERT_EQ(bv, (std::vector<dynamic_t> { 4, 4, 4 }));
        ASSERT_EQ(cv, (std::vector<dynamic_t> { 4, 4, 4 }));
    }

    c = a;

    {
        ASSERT_EQ(a.width(), 2);
        ASSERT_EQ(b.width(), 64);
        ASSERT_EQ(c.width(), 2);

        std::vector<dynamic_t> av(a.begin(), a.end());
        std::vector<dynamic_t> bv(b.begin(), b.end());
        std::vector<dynamic_t> cv(c.begin(), c.end());

        ASSERT_EQ(av, (std::vector<dynamic_t> { 3, 3, 3 }));
        ASSERT_EQ(bv, (std::vector<dynamic_t> { 4, 4, 4 }));
        ASSERT_EQ(cv, (std::vector<dynamic_t> { 3, 3, 3 }));
    }

    a.width(3);
    {
        ASSERT_EQ(a.width(), 3);
        std::vector<dynamic_t> av(a.begin(), a.end());
        ASSERT_EQ(av, (std::vector<dynamic_t> { 3, 3, 3 }));
    }
    a.push_back(7);
    a.push_back(8);
    a.push_back(15);
    {
        std::vector<dynamic_t> av(a.begin(), a.end());
        ASSERT_EQ(av, (std::vector<dynamic_t> { 3, 3, 3, 7, 0, 7 }));
    }
    a.width(2);
    {
        std::vector<dynamic_t> av(a.begin(), a.end());
        ASSERT_EQ(av, (std::vector<dynamic_t> { 3, 3, 3, 3, 0, 3 }));
    }
    a.width(1);
    {
        std::vector<dynamic_t> av(a.begin(), a.end());
        ASSERT_EQ(av, (std::vector<dynamic_t> { 1, 1, 1, 1, 0, 1 }));
    }

    IntVector<dynamic_t> d { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    d.width(4);

    ASSERT_EQ(d.size(), 10);
    ASSERT_EQ(d.width(), 4);
    auto dbs = d.bit_size();
    auto dbc = d.bit_capacity();
    {
        std::vector<dynamic_t> dv(d.begin(), d.end());
        ASSERT_EQ(dv, (std::vector<dynamic_t> { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }));
    }

    d.resize(5, 0, 8);

    ASSERT_EQ(d.size(), 5);
    ASSERT_EQ(d.width(), 8);
    ASSERT_EQ(dbs, d.bit_size());
    ASSERT_EQ(dbc, d.bit_capacity());
    {
        std::vector<dynamic_t> dv(d.begin(), d.end());
        ASSERT_EQ(dv, (std::vector<dynamic_t> { 1, 2, 3, 4, 5 }));
    }

    d.resize(10, 11, 4);

    ASSERT_EQ(d.size(), 10);
    ASSERT_EQ(d.width(), 4);
    ASSERT_EQ(dbs, d.bit_size());
    ASSERT_EQ(dbc, d.bit_capacity());
    {
        std::vector<dynamic_t> dv(d.begin(), d.end());
        ASSERT_EQ(dv, (std::vector<dynamic_t> { 1, 2, 3, 4, 5, 11, 11, 11, 11, 11 }));
    }

    d.resize(5, 11, 2);

    ASSERT_EQ(d.size(), 5);
    ASSERT_EQ(d.width(), 2);
    ASSERT_EQ(10, d.bit_size());
    {
        std::vector<dynamic_t> dv(d.begin(), d.end());
        ASSERT_EQ(dv, (std::vector<dynamic_t> { 1, 2, 3, 0, 1 }));
    }

    d.resize(11, 20, 10);

    ASSERT_EQ(d.size(), 11);
    ASSERT_EQ(d.width(), 10);
    ASSERT_EQ(110, d.bit_size());
    {
        std::vector<dynamic_t> dv(d.begin(), d.end());
        ASSERT_EQ(dv, (std::vector<dynamic_t> { 1, 2, 3, 0, 1, 20, 20, 20, 20, 20, 20 }));
    }

    IntVector<dynamic_t> e;
    e.bit_reserve(1024);
    ASSERT_GE(e.bit_capacity(), 1024);

}

template<size_t N>
void generic_int_vector_trait_template() {
    using namespace int_vector;

    typedef typename IntVectorTrait<uint_t<N>>::backing_data T;

    T t = T();

    ASSERT_EQ(t.backing2bits(1), 64);
    ASSERT_EQ(t.backing2bits(10), 640);

    ASSERT_EQ(t.bits2backing(0), 0);
    ASSERT_EQ(t.bits2backing(1), 1);
    ASSERT_EQ(t.bits2backing(7), 1);
    ASSERT_EQ(t.bits2backing(63), 1);
    ASSERT_EQ(t.bits2backing(64), 1);
    ASSERT_EQ(t.bits2backing(65), 2);
    ASSERT_EQ(t.bits2backing(127), 2);
    ASSERT_EQ(t.bits2backing(128), 2);
    ASSERT_EQ(t.bits2backing(129), 3);

    ASSERT_EQ(t.elem2bits(1), N);
    ASSERT_EQ(t.elem2bits(10), N * 10);
}

TEST(generic_int_vector_trait_template, N9) {
    generic_int_vector_trait_template<9>();
}

TEST(generic_int_vector_trait_template, N7) {
    generic_int_vector_trait_template<7>();
}

TEST(generic_int_vector_trait_template, N1) {
    generic_int_vector_trait_template<1>();
}
