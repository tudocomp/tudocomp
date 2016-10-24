#ifndef UINT_T_HPP
#define UINT_T_HPP

#include <tudocomp/util/IntegerBase.hpp>

#include <cstdint>

using tdc::IntegerBase;

/** class for storing integers of arbitrary bits.
 * Useful values are 40,48, and 56.
 * Standard value is 40 bits since we can store values up to 1TiB and
 * address values up to 1TB with 40-bits integers and 40-bits pointers, respectively.
 */
template<size_t bits>
class uint_t: public IntegerBase<uint_t<bits>> {
    static_assert(bits > 0, "bits must be non-negative");
    static_assert(bits < 65, "bits must be at most 64");
    uint64_t m_data : bits;

    friend class tdc::IntegerBaseTrait<uint_t<bits>>;

public:
    uint_t() {}
    uint_t(const uint_t&& i): m_data(i.m_data) {}

    // copying
    inline uint_t(const uint_t& i): m_data(i.m_data) {}
    inline uint_t& operator=(const uint_t& b) { m_data = b.data; return *this; }

    // 64 bit conversions
    inline uint_t(uint64_t i): m_data(i) {}
    inline uint_t& operator=(uint64_t data) { m_data = data; return *this; }
    inline operator uint64_t() const { return m_data; }

    // 32 bit conversions
    inline uint_t(uint32_t i): m_data(i) {}
    inline uint_t& operator=(uint32_t data) { m_data = data; return *this; }
    inline operator uint32_t() const { return m_data; }

    // compatibility with unsuffixed integer literals
    inline uint_t(int i): m_data(i) {}
    inline uint_t& operator=(int data) { m_data = data; return *this; }
    inline operator int() const { return m_data; }

    // To enable interop with gtest
    inline operator long long int() const { return m_data; }

} __attribute__((packed));

namespace tdc {
    template<size_t N>
    struct IntegerBaseTrait<uint_t<N>, typename std::enable_if<(N <= 32)>::type> {
        typedef uint32_t SelfMaxBit;

        inline static void assign(uint_t<N>& self, uint32_t v) {
            self.m_data = v;
        }

        inline static void assign(uint_t<N>& self, uint64_t v) {
            self.m_data = v;
        }

        inline static SelfMaxBit cast_for_self_op(const uint_t<N>& self) {
            return self.m_data;
        }

        inline static SelfMaxBit cast_for_32_op(const uint_t<N>& self) {
            return self.m_data;
        }

        inline static uint64_t cast_for_64_op(const uint_t<N>& self) {
            return self.m_data;
        }
    };

    template<size_t N>
    struct IntegerBaseTrait<uint_t<N>, typename std::enable_if<(N > 32)>::type> {
        typedef uint64_t SelfMaxBit;

        inline static void assign(uint_t<N>& self, uint32_t v) {
            self.m_data = v;
        }

        inline static void assign(uint_t<N>& self, uint64_t v) {
            self.m_data = v;
        }

        inline static SelfMaxBit cast_for_self_op(const uint_t<N>& self) {
            return self.m_data;
        }

        inline static SelfMaxBit cast_for_32_op(const uint_t<N>& self) {
            return self.m_data;
        }

        inline static uint64_t cast_for_64_op(const uint_t<N>& self) {
            return self.m_data;
        }
    };
}

static_assert(sizeof(uint_t<8>)  == 1, "sanity check");
static_assert(sizeof(uint_t<16>) == 2, "sanity check");
static_assert(sizeof(uint_t<24>) == 3, "sanity check");
static_assert(sizeof(uint_t<32>) == 4, "sanity check");
static_assert(sizeof(uint_t<40>) == 5, "sanity check");
static_assert(sizeof(uint_t<48>) == 6, "sanity check");
static_assert(sizeof(uint_t<56>) == 7, "sanity check");
static_assert(sizeof(uint_t<64>) == 8, "sanity check");

static_assert(sizeof(uint_t<7>)  == 1, "sanity check");
static_assert(sizeof(uint_t<15>) == 2, "sanity check");
static_assert(sizeof(uint_t<23>) == 3, "sanity check");
static_assert(sizeof(uint_t<31>) == 4, "sanity check");
static_assert(sizeof(uint_t<39>) == 5, "sanity check");
static_assert(sizeof(uint_t<47>) == 6, "sanity check");
static_assert(sizeof(uint_t<55>) == 7, "sanity check");
static_assert(sizeof(uint_t<63>) == 8, "sanity check");

static_assert(sizeof(uint_t<9>)  == 2, "sanity check");
static_assert(sizeof(uint_t<17>) == 3, "sanity check");
static_assert(sizeof(uint_t<25>) == 4, "sanity check");
static_assert(sizeof(uint_t<33>) == 5, "sanity check");
static_assert(sizeof(uint_t<41>) == 6, "sanity check");
static_assert(sizeof(uint_t<49>) == 7, "sanity check");
static_assert(sizeof(uint_t<57>) == 8, "sanity check");

#endif /* UINT_T_HPP */
