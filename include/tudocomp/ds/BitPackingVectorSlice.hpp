#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <climits>

#include <tudocomp/ds/IntPtr.hpp>
#include <tudocomp/ds/uint_t.hpp>
#include <tudocomp/ds/dynamic_t.hpp>
#include <tudocomp/util/IntegerBase.hpp>

#include <glog/logging.h>

#include <tudocomp/ds/IntVector.hpp>

/// \cond INTERNAL

namespace tdc {
namespace int_vector {
    template<class T>
    struct BitPackingVectorSliceBase {};

    template<>
    struct BitPackingVectorSliceBase<dynamic_t> {
        typedef DynamicIntValueType internal_data_type;
        typedef dynamic_t           value_type;

        ConstGenericView<internal_data_type> m_vec;
        uint64_t m_real_size;
        uint8_t m_width;
        uint8_t m_offset;

        inline BitPackingVectorSliceBase():
            m_vec(), m_real_size(0), m_width(64), m_offset(0) {}
        inline BitPackingVectorSliceBase(const BitPackingVectorSliceBase& other):
            m_vec(other.m_vec), m_real_size(other.m_real_size), m_width(other.m_width), m_offset(other.m_offset) {}

        inline uint8_t raw_width() const { return m_width; }
        inline void set_width_raw(uint8_t width) { m_width = width; }

    };

    template<class T>
    struct BitPackingVectorSlice: BitPackingVectorSliceBase<T> {
        typedef typename BitPackingVectorSliceBase<T>::value_type            value_type;

        typedef ConstIntRef<value_type>                              const_reference;

        typedef ConstIntPtr<value_type>                              const_pointer;

        typedef const_pointer                                        const_iterator;

        typedef typename std::reverse_iterator<const_iterator>       const_reverse_iterator;

        typedef ptrdiff_t                                            difference_type;
        typedef size_t                                               size_type;

        typedef typename BitPackingVectorSliceBase<T>::internal_data_type    internal_data_type;

        template<class M>
        friend bool operator==(const BitPackingVectorSlice<M>& lhs, const BitPackingVectorSlice<M>& rhs);

        inline static uint64_t backing2bits_w(size_t n) {
            return uint64_t(sizeof(internal_data_type) * CHAR_BIT) * uint64_t(n);
        }
        inline uint64_t backing2bits(size_t n) const {
            return backing2bits_w(n);
        }

        inline static uint64_t elem2bits_w(size_t n, uint8_t w) {
            return uint64_t(w) * uint64_t(n);
        }
        inline uint64_t elem2bits(size_t n) const {
            return elem2bits_w(n, this->width());
        }

        inline static uint64_t bits2backing_w(uint64_t bits) {
            if (bits == 0) {
                return 0;
            }
            return ((bits - 1) / backing2bits_w(1)) + 1;
        }
        inline uint64_t bits2backing(uint64_t bits) const {
            return bits2backing_w(bits);
        }

        inline static uint64_t bits2elem_w(uint64_t bits, uint8_t w) {
            if (bits == 0) {
                return 0;
            }
            return ((bits - 1) / elem2bits_w(1, w)) + 1;
        }
        inline uint64_t bits2elem(uint64_t bits) const {
            return bits2elem_w(bits, this->width());
        }

        struct PosAndOffset { size_t pos; uint8_t offset; };
        inline static PosAndOffset bitpos2backingpos_w(uint64_t bits) {
            return PosAndOffset {
                bits / backing2bits_w(1),
                uint8_t(bits % backing2bits_w(1))
            };
        }
        inline PosAndOffset bitpos2backingpos(uint64_t bits) const {
            return bitpos2backingpos_w(bits);
        }

        inline explicit BitPackingVectorSlice(): BitPackingVectorSliceBase<T>::BitPackingVectorSliceBase() {}

        inline BitPackingVectorSlice(ConstGenericView<internal_data_type> raw, size_type n, uint8_t width, uint8_t offset):
            BitPackingVectorSlice()
        {
            this->m_vec = raw;
            this->m_real_size = n;
            this->m_offset = offset;
            this->set_width_raw(width);
        }

        inline BitPackingVectorSlice(const IntVector<dynamic_t>& vec):
            BitPackingVectorSlice(ConstGenericView<internal_data_type>(vec.data(), bits2backing_w(elem2bits_w(vec.size(), vec.width()))), vec.size(), vec.width(), 0)
        {}

        inline BitPackingVectorSlice (const BitPackingVectorSlice& other):
            BitPackingVectorSliceBase<T>(other) {}

        inline BitPackingVectorSlice& operator=(const BitPackingVectorSlice& other) {
            this->m_vec = other.m_vec;
            this->m_real_size = other.m_real_size;
            this->m_offset = other.m_offset;
            this->set_width_raw(other.width());
            return *this;
        }

        inline const_iterator begin() const {
            using Base = typename int_vector::IntPtrBase<const_pointer>;
            auto x = bitpos2backingpos(this->m_offset);
            return const_pointer(Base(this->m_vec.data() + x.pos, x.offset, this->width()));
        }

        inline const_iterator end() const {
            using Base = typename int_vector::IntPtrBase<const_pointer>;
            auto x = bitpos2backingpos(this->m_offset + elem2bits(this->m_real_size));
            return const_pointer(Base(this->m_vec.data() + x.pos, x.offset, this->width()));
        }

        inline const_reverse_iterator rbegin() const {
            return std::reverse_iterator<const_iterator>(end());
        }

        inline const_reverse_iterator rend() const {
            return std::reverse_iterator<const_iterator>(begin());
        }

        inline const_iterator cbegin() const {
            return begin();
        }

        inline const_iterator cend() const {
            return end();
        }

        inline const_reverse_iterator crbegin() const {
            return rbegin();
        }

        inline const_reverse_iterator crend() const {
            return rend();
        }

        inline size_type size() const {
            return this->m_real_size;
        }

        inline uint64_t bit_size() const {
            return elem2bits(this->m_real_size);
        }

        inline size_type max_size() const {
            // Empty vector does not allocate, so this is fine
            return size();
        }

        inline bool empty() const {
            return size() == 0;
        }

        inline const_reference operator[](size_type n) const {
            using Base = typename int_vector::IntPtrBase<const_pointer>;
            DCHECK(n < size());
            auto x = bitpos2backingpos(this->m_offset + elem2bits(n));
            return const_reference(const_pointer(Base(this->m_vec.data() + x.pos, x.offset, this->width())));
        }

        inline void range_check(size_type n) const {
            if (n >= size()) {
                std::stringstream ss;
                ss << "Out-of-range access of IntVector: index is ";
                ss << n;
                ss << ", size() is ";
                ss << size();
                throw std::out_of_range(ss.str());
            }
        }

        inline const_reference at(size_type n) const {
            range_check(n);
            return operator[](n);
        }

        inline const_reference front() const {
            return operator[](0);
        }

        inline const_reference back() const {
            return operator[](size() - 1);
        }

        inline uint8_t width() const {
            return this->raw_width();
        }

        inline BitPackingVectorSlice slice(size_t from, size_t to = size_t(-1)) const {
            if (to == size_t(-1)) {
                to = size();
            }

            auto from_bits = bitpos2backingpos(this->m_offset + elem2bits(from));
            auto to_bits = bitpos2backingpos(this->m_offset + elem2bits(to));

            auto from_idx = from_bits.pos;
            auto to_idx = to_bits.pos;
            if (to_bits.offset != 0) {
                to_idx += 1;
            }

            auto raw = this->m_vec.slice(from_idx, to_idx);

            return BitPackingVectorSlice(raw, to - from, width(), from_bits.offset);
        }
    };

    template<class N>
    bool operator==(const BitPackingVectorSlice<N>& lhs, const BitPackingVectorSlice<N>& rhs) {
        DCHECK(lhs.width() == rhs.width());
        if (lhs.size() == rhs.size()) {
            return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin());
        }
        return false;
    }

    template<class N>
    bool operator!=(const BitPackingVectorSlice<N>& lhs, const BitPackingVectorSlice<N>& rhs) {
        return !(lhs == rhs);
    }

    template<class N>
    bool operator<(const BitPackingVectorSlice<N>& lhs, const BitPackingVectorSlice<N>& rhs) {
        return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
    }

    template<class N>
    bool operator<=(const BitPackingVectorSlice<N>& lhs, const BitPackingVectorSlice<N>& rhs) {
        return !(lhs > rhs);
    }

    template<class N>
    bool operator>(const BitPackingVectorSlice<N>& lhs, const BitPackingVectorSlice<N>& rhs) {
        return std::lexicographical_compare(rhs.cbegin(), rhs.cend(), lhs.cbegin(), lhs.cend());
    }

    template<class N>
    bool operator>=(const BitPackingVectorSlice<N>& lhs, const BitPackingVectorSlice<N>& rhs) {
        return !(lhs < rhs);
    }
}

template<class T>
using BitPackingVectorSlice = int_vector::BitPackingVectorSlice<T>;

}

/// \endcond

