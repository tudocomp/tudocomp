#pragma once

#include <type_traits>
#include <vector>

#include <tudocomp/util/bits.hpp>
#include <tudocomp/ds/uint_t.hpp>
#include <tudocomp/ds/dynamic_t.hpp>

namespace tdc {namespace int_vector {
    enum class ElementStorageMode {
        Direct,
        BitPacked
    };

    /// \cond INTERNAL
    using DynamicIntValueType = uint64_t;

    struct DynamicWidthMemRw {
        inline static void write_int(uint64_t* word, uint64_t x, uint8_t offset, const uint8_t len) {
            sdsl_bits::write_int(word, x, offset, len);
        }
        inline static uint64_t read_int(const uint64_t* word, uint8_t offset, const uint8_t len) {
            return sdsl_bits::read_int(word, offset, len);
        }
    };

    struct BitWidthMemRw {
        inline static void write_int(uint64_t* word, uint64_t v, uint8_t o, const uint8_t) {
            auto& p = *word;
            const auto mask = uint64_t(1) << o;

            v &= 1;
            p &= (~mask);
            p |= (uint64_t(v) << o);
        }
        inline static uint64_t read_int(const uint64_t* word, uint8_t o, const uint8_t) {
            const auto p = *word;
            const auto mask = uint64_t(1) << o;

            return (p & mask) != 0;
        }
    };

    template<class MB, class MemRw>
    struct RefDispatch {
        typedef MB SelfMaxBit;

        template<class Ref, class V>
        inline static void assign(Ref& self, V v) {
            MemRw::write_int(
                self.m_ptr.m_ptr,
                v,
                self.m_ptr.m_bit_offset,
                self.m_ptr.data_bit_size()
            );
        }

        template<class Ref, class R>
        inline static R cast_for_op(const Ref& self) {
            return MemRw::read_int(
                self.m_ptr.m_ptr,
                self.m_ptr.m_bit_offset,
                self.m_ptr.data_bit_size()
            );
        }
    };

    template<size_t N>
    class FixedWidthRepr {
    public:
        constexpr FixedWidthRepr(uint8_t) {}
        constexpr uint8_t data_bit_size() const { return N; }
        constexpr void set_data_bit_size(uint8_t) { }

        static constexpr uint8_t width_from_value(uint64_t value) {
            return 0;
        }
    };

    class DynamicWidthRepr {
        uint8_t m_bit_size;
    public:
        constexpr DynamicWidthRepr(uint8_t bit_size): m_bit_size(bit_size) {}
        constexpr uint8_t data_bit_size() const { return m_bit_size; }
        constexpr void set_data_bit_size(uint8_t bit_size) { m_bit_size = bit_size; }

        static constexpr uint8_t width_from_value(uint64_t value) {
            return bits_for(value);
        }
    };

    template<typename const_qual_val_t, size_t N>
    class IntPtrFixedWidthRepr: public FixedWidthRepr<N> {
    public:
        const_qual_val_t* m_ptr;
        uint8_t m_bit_offset;
    public:
        IntPtrFixedWidthRepr(const_qual_val_t* ptr, uint8_t offset, uint8_t size):
             FixedWidthRepr<N>(size), m_ptr(ptr), m_bit_offset(offset) {}
        inline IntPtrFixedWidthRepr data_offset_to(const_qual_val_t* ptr, uint8_t offset) const {
            return IntPtrFixedWidthRepr(ptr, offset, this->data_bit_size());
        }
    };
    template<typename const_qual_val_t>
    class IntPtrDynamicWidthRepr: public DynamicWidthRepr {
    public:
        const_qual_val_t* m_ptr;
        uint8_t m_bit_offset;

        using WidthRepr = DynamicWidthRepr;
    public:
        IntPtrDynamicWidthRepr(const_qual_val_t* ptr, uint8_t offset, uint8_t size):
            DynamicWidthRepr(size), m_ptr(ptr), m_bit_offset(offset) {}
        inline IntPtrDynamicWidthRepr data_offset_to(const_qual_val_t* ptr, uint8_t offset) const {
            return IntPtrDynamicWidthRepr(ptr, offset, this->data_bit_size());
        }
    };
    template<size_t N>
    struct FixedBitPackingVectorRepr {
        using internal_data_type = DynamicIntValueType;

        std::vector<internal_data_type> m_vec;
        uint64_t m_real_size;

        inline FixedBitPackingVectorRepr():
            m_vec(), m_real_size(0) {}
        inline FixedBitPackingVectorRepr(const FixedBitPackingVectorRepr& other):
            m_vec(other.m_vec), m_real_size(other.m_real_size) {}
        inline FixedBitPackingVectorRepr(FixedBitPackingVectorRepr&& other):
            m_vec(std::move(other.m_vec)), m_real_size(other.m_real_size) {}

        inline uint8_t raw_width() const { return N; }
        inline void set_width_raw(uint8_t) { }
    };
    struct DynamicBitPackingVectorRepr {
        using internal_data_type = DynamicIntValueType;

        std::vector<internal_data_type> m_vec;
        uint64_t m_real_size;
        uint8_t m_width;

        inline DynamicBitPackingVectorRepr():
            m_vec(), m_real_size(0), m_width(64) {}
        inline DynamicBitPackingVectorRepr(const DynamicBitPackingVectorRepr& other):
            m_vec(other.m_vec), m_real_size(other.m_real_size), m_width(other.m_width) {}
        inline DynamicBitPackingVectorRepr(DynamicBitPackingVectorRepr&& other):
            m_vec(std::move(other.m_vec)), m_real_size(other.m_real_size), m_width(other.m_width) {}

        inline uint8_t raw_width() const { return m_width; }
        inline void set_width_raw(uint8_t width) { m_width = width; }
    };

    template<typename T, typename X = void>
    struct IntRepr {};

    template<size_t N>
    struct IntRepr<uint_impl_t<N>, typename std::enable_if_t<(N > 1) && (N <= 32)>> {
        using value_type = uint_impl_t<N>;
        using mem_rw = DynamicWidthMemRw;
        using IntOpDispatch = RefDispatch<uint32_t, mem_rw>;

        using WidthRepr = FixedWidthRepr<N>;

        using ConstIntPtrBase = IntPtrFixedWidthRepr<DynamicIntValueType const, N>;
        using IntPtrBase = IntPtrFixedWidthRepr<DynamicIntValueType, N>;

        using BitPackingVectorRepr = FixedBitPackingVectorRepr<N>;
    };

    template<size_t N>
    struct IntRepr<uint_impl_t<N>, typename std::enable_if_t<(N > 32) && (N <= 64)>> {
        using value_type = uint_impl_t<N>;
        using mem_rw = DynamicWidthMemRw;
        using IntOpDispatch = RefDispatch<uint64_t, mem_rw>;

        using WidthRepr = FixedWidthRepr<N>;

        using ConstIntPtrBase = IntPtrFixedWidthRepr<DynamicIntValueType const, N>;
        using IntPtrBase = IntPtrFixedWidthRepr<DynamicIntValueType, N>;

        using BitPackingVectorRepr = FixedBitPackingVectorRepr<N>;
    };

    template<>
    struct IntRepr<bool> {
        using value_type = bool;
        using mem_rw = BitWidthMemRw;
        using IntOpDispatch = RefDispatch<uint32_t, mem_rw>;

        using WidthRepr = FixedWidthRepr<1>;

        using ConstIntPtrBase = IntPtrFixedWidthRepr<DynamicIntValueType const, 1>;
        using IntPtrBase = IntPtrFixedWidthRepr<DynamicIntValueType, 1>;

        using BitPackingVectorRepr = FixedBitPackingVectorRepr<1>;
    };

    template<>
    struct IntRepr<dynamic_t> {
        using value_type = uint64_t;
        using mem_rw = DynamicWidthMemRw;
        using IntOpDispatch = RefDispatch<uint64_t, mem_rw>;

        using WidthRepr = DynamicWidthRepr;

        using ConstIntPtrBase = IntPtrDynamicWidthRepr<DynamicIntValueType const>;
        using IntPtrBase = IntPtrDynamicWidthRepr<DynamicIntValueType>;

        using BitPackingVectorRepr = DynamicBitPackingVectorRepr;
    };

    template<size_t N>
    struct IntRepr<uint_impl_t<N>, typename std::enable_if_t<(N <= 1)>>; // Unused

    template<size_t N>
    struct IntRepr<uint_impl_t<N>, typename std::enable_if_t<(N > 64)>>; // Invalid

    ///\endcond
}}
