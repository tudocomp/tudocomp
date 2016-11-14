#ifndef UINT_T_HPP
#define UINT_T_HPP

#include <tudocomp/util/IntegerBase.hpp>

#include <cstdint>

namespace tdc {

template<size_t bits>
class uint_t;

template<class MB>
struct UinttDispatch {
    typedef MB SelfMaxBit;

    template<class Ref, class V>
    inline static void assign(Ref& self, V v) {
        self.m_data = v;
    };

    template<class Ref, class R>
    inline static R cast_for_op(const Ref& self) {
        return self.m_data;
    }
};

template<size_t N>
struct ConstIntegerBaseTrait<uint_t<N>, typename std::enable_if<(N <= 32)>::type> {
    typedef UinttDispatch<uint32_t> Dispatch;
};

template<size_t N>
struct IntegerBaseTrait<uint_t<N>, typename std::enable_if<(N <= 32)>::type>
: ConstIntegerBaseTrait<uint_t<N>> {
    typedef UinttDispatch<uint32_t> Dispatch;
};

template<size_t N>
struct ConstIntegerBaseTrait<uint_t<N>, typename std::enable_if<(N > 32)>::type> {
    typedef UinttDispatch<uint64_t> Dispatch;
};

template<size_t N>
struct IntegerBaseTrait<uint_t<N>, typename std::enable_if<(N > 32)>::type>
: ConstIntegerBaseTrait<uint_t<N>> {
    typedef UinttDispatch<uint64_t> Dispatch;
};

/// Custom integer type for storing values of arbitrary bit size `bits`.
///
/// It is guaranteed that this type will only have a byte size as large
/// as needed to store all bits, but there will be padding bits for values
/// not multiples of 8.
///
/// In practice useful sizes are 40, 48 and 56 bits.
/// 40 bit indices correspond to 1TiB of addressable memory,
/// and 48 bit correspond to today's hardware limits of the x86_64 architecture.
template<size_t bits>
class uint_t: public IntegerBase<uint_t<bits>> {
    static_assert(bits > 0, "bits must be non-negative");
    static_assert(bits < 65, "bits must be at most 64");
    uint64_t m_data: bits;

    friend class UinttDispatch<uint32_t>;
    friend class UinttDispatch<uint64_t>;

public:
    uint_t(): m_data(0) {}
    uint_t(uint_t&& i): m_data(i.m_data) {}

    // copying
    inline uint_t(const uint_t& i): m_data(i.m_data) {}
    inline uint_t& operator=(const uint_t& b) { m_data = b.m_data; return *this; }

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

}

#endif /* UINT_T_HPP */
