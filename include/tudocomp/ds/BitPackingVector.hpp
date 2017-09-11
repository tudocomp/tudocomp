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

#include <sdsl/bits.hpp>
#include <glog/logging.h>

///  \cond INTERNAL

namespace tdc {
namespace int_vector {
    enum class ElementStorageMode {
        Direct,
        BitPacked
    };

    inline void width_error() {
        throw std::runtime_error("Can not set the width of a IntVector with statically sized elements");
    }

    template<class T>
    struct BitPackingVectorBase {};

    template<size_t N>
    struct BitPackingVectorBase<uint_t<N>> {
        typedef DynamicIntValueType internal_data_type;
        typedef uint_t<N>           value_type;

        std::vector<internal_data_type> m_vec;
        uint64_t m_real_size;

        inline BitPackingVectorBase():
            m_vec(), m_real_size(0) {}
        inline BitPackingVectorBase(const BitPackingVectorBase& other):
            m_vec(other.m_vec), m_real_size(other.m_real_size) {}
        inline BitPackingVectorBase(BitPackingVectorBase&& other):
            m_vec(std::move(other.m_vec)), m_real_size(other.m_real_size) {}

        inline uint8_t raw_width() const { return N; }
        inline void set_width_raw(uint8_t) { }

    };

    template<>
    struct BitPackingVectorBase<dynamic_t> {
        typedef DynamicIntValueType internal_data_type;
        typedef dynamic_t           value_type;

        std::vector<internal_data_type> m_vec;
        uint64_t m_real_size;
        uint8_t m_width;

        inline BitPackingVectorBase():
            m_vec(), m_real_size(0), m_width(64) {}
        inline BitPackingVectorBase(const BitPackingVectorBase& other):
            m_vec(other.m_vec), m_real_size(other.m_real_size), m_width(other.m_width) {}
        inline BitPackingVectorBase(BitPackingVectorBase&& other):
            m_vec(std::move(other.m_vec)), m_real_size(other.m_real_size), m_width(other.m_width) {}

        inline uint8_t raw_width() const { return m_width; }
        inline void set_width_raw(uint8_t width) { m_width = width; }

    };

    template<class T>
    struct BitPackingVector: BitPackingVectorBase<T> {
        typedef typename BitPackingVectorBase<T>::value_type            value_type;

        typedef IntRef<value_type>                                   reference;
        typedef ConstIntRef<value_type>                              const_reference;

        typedef IntPtr<value_type>                                   pointer;
        typedef ConstIntPtr<value_type>                              const_pointer;

        typedef pointer                                              iterator;
        typedef const_pointer                                        const_iterator;

        typedef typename std::reverse_iterator<iterator>             reverse_iterator;
        typedef typename std::reverse_iterator<const_iterator>       const_reverse_iterator;

        typedef ptrdiff_t                                            difference_type;
        typedef size_t                                               size_type;

        typedef typename BitPackingVectorBase<T>::internal_data_type    internal_data_type;

        template<class M>
        friend bool operator==(const BitPackingVector<M>& lhs, const BitPackingVector<M>& rhs);

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

        inline explicit BitPackingVector(): BitPackingVectorBase<T>::BitPackingVectorBase() {}
        inline explicit BitPackingVector(size_type n): BitPackingVector() {
            this->m_real_size = n;
            size_t converted_size = bits2backing(elem2bits(this->m_real_size));
            this->m_vec = std::vector<internal_data_type>(converted_size);
            DCHECK_EQ(converted_size, this->m_vec.capacity());
        }
        inline BitPackingVector(size_type n, const value_type& val): BitPackingVector(n) {
            auto ptr = this->m_vec.data();
            uint8_t offset = 0;

            for (size_t i = 0; i < n; i++) {
                bits::write_int_and_move(ptr, val, offset, this->width());
            }
        }

        inline BitPackingVector(size_type n, const value_type& val, uint8_t width):
            BitPackingVector()
        {
            this->set_width_raw(width);
            this->reserve(n);
            this->resize(n, val);
            DCHECK_EQ(this->m_vec.size(), bits2backing(elem2bits(n)));
            DCHECK_EQ(this->m_vec.size(), this->m_vec.capacity());
        }

        template <class InputIterator>
        inline BitPackingVector(InputIterator first, InputIterator last) {
            // TODO: specialize for random access iterator
            for(; first != last; first++) {
                push_back(*first);
            }
        }
        inline BitPackingVector (const BitPackingVector& other):
            BitPackingVectorBase<T>(other) {}
        inline BitPackingVector (BitPackingVector&& other):
            BitPackingVectorBase<T>(std::move(other)) {}
        inline BitPackingVector(std::initializer_list<value_type> il):
            BitPackingVector(il.begin(), il.end()) {}

        inline BitPackingVector& operator=(const BitPackingVector& other) {
            this->m_vec = other.m_vec;
            this->m_real_size = other.m_real_size;
            this->set_width_raw(other.width());
            return *this;
        }

        inline BitPackingVector& operator=(BitPackingVector&& other) {
            this->m_vec = std::move(other.m_vec);
            this->m_real_size = other.m_real_size;
            this->set_width_raw(other.width());
            return *this;
        }

        inline BitPackingVector& operator=(std::initializer_list<value_type> il) {
            *this = BitPackingVector(il);
            return *this;
        }

        inline iterator begin() {
            using Data = typename int_vector::IntPtrTrait<pointer>::Data;
            auto x = bitpos2backingpos(0);
            return pointer(Data(&this->m_vec[x.pos], x.offset, this->width()));
        }

        inline iterator end() {
            using Data = typename int_vector::IntPtrTrait<pointer>::Data;
            auto x = bitpos2backingpos(elem2bits(this->m_real_size));
            return pointer(Data(&this->m_vec[x.pos], x.offset, this->width()));
        }

        inline reverse_iterator rbegin() {
            return std::reverse_iterator<iterator>(end());
        }

        inline reverse_iterator rend() {
            return std::reverse_iterator<iterator>(begin());
        }

        inline const_iterator begin() const {
            using Data = typename int_vector::IntPtrTrait<const_pointer>::Data;
            auto x = bitpos2backingpos(0);
            return const_pointer(Data(&this->m_vec[x.pos], x.offset, this->width()));
        }

        inline const_iterator end() const {
            using Data = typename int_vector::IntPtrTrait<const_pointer>::Data;
            auto x = bitpos2backingpos(elem2bits(this->m_real_size));
            return const_pointer(Data(&this->m_vec[x.pos], x.offset, this->width()));
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
            return std::vector<value_type>().max_size();
        }

        inline void resize(size_type n) {
            this->m_real_size = n;
            this->m_vec.resize(bits2backing(elem2bits(n)), 0);
        }

        inline void resize(size_type n, const value_type& val) {
            auto old_size = size();
            resize(n);
            if (old_size < n) {
                for (auto a = begin() + old_size, b = end(); a != b; ++a) {
                    *a = val;
                }
            }
        }

        inline size_type capacity() const {
            return bits2elem(backing2bits(this->m_vec.capacity()));
        }

        inline bool empty() const {
            return size() == 0;
        }

        inline void reserve(size_type n) {
           this->m_vec.reserve(bits2backing(elem2bits(n)));
        }

        inline void shrink_to_fit() {
            this->m_vec.shrink_to_fit();
        }

        inline reference operator[](size_type n) {
            using Data = typename int_vector::IntPtrTrait<pointer>::Data;
            DCHECK_LT(n, size());
            auto x = bitpos2backingpos(elem2bits(n));
            return reference(pointer(Data(this->m_vec.data() + x.pos, x.offset, this->width())));
        }

        inline const_reference operator[](size_type n) const {
            using Data = typename int_vector::IntPtrTrait<const_pointer>::Data;
            DCHECK(n < size());
            auto x = bitpos2backingpos(elem2bits(n));
            return const_reference(const_pointer(Data(this->m_vec.data() + x.pos, x.offset, this->width())));
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

        inline reference at(size_type n) {
            range_check(n);
            return operator[](n);
        }

        inline const_reference at(size_type n) const {
            range_check(n);
            return operator[](n);
        }

        inline reference front() {
            return operator[](0);
        }

        inline const_reference front() const {
            return operator[](0);
        }

        inline reference back() {
            return operator[](size() - 1);
        }

        inline const_reference back() const {
            return operator[](size() - 1);
        }

        inline internal_data_type* data() noexcept {
            return this->m_vec.data();
        }

        inline const internal_data_type* data() const noexcept {
            return this->m_vec.data();
        }

        template <class InputIterator>
        inline void assign(InputIterator first, InputIterator last) {
            *this = BitPackingVector(first, last);
        }

        inline void assign(size_type n, const value_type& val) {
            *this = BitPackingVector(n, val);
        }

        inline void assign(std::initializer_list<value_type> il) {
            *this = BitPackingVector(il);
        }

        inline void push_back(const value_type& val) {
            this->m_real_size += 1;

            while (elem2bits(this->m_real_size) > backing2bits(this->m_vec.size())) {
                this->m_vec.push_back(0);
            }

            back() = val;
        }

        inline void push_back(value_type&& val) {
            const auto& r = val;
            push_back(r);
        }

        inline void pop_back() {
            DCHECK(!empty());
            this->m_real_size -= 1;
            while (bits2backing(elem2bits(this->m_real_size)) < this->m_vec.size()) {
                this->m_vec.pop_back();
            }
        }

        inline iterator insert(const_iterator position, const value_type& val) {
            return insert(position, size_type(1), val);
        }

        inline iterator insert(const_iterator position, size_type n, const value_type& val) {
            // Remember element offset before iterator invalidation
            auto p = (position - cbegin());

            // Step 1: Grow backing vector by needed amount
            {
                auto new_bits_needed = elem2bits(n);
                auto existing_extra_bits = backing2bits(this->m_vec.size()) - elem2bits(this->m_real_size);

                if (new_bits_needed > existing_extra_bits) {
                    new_bits_needed -= existing_extra_bits;
                } else {
                    new_bits_needed = 0;
                }

                auto new_backing_needed = bits2backing(new_bits_needed);
                this->m_vec.insert(this->m_vec.cend(), new_backing_needed, 0);
                this->m_real_size += n;
            }

            // Step 2: move elements to back, leaving a gap
            {
                auto start = begin() + p;
                auto old_end = end() - n;
                auto new_end = end();

                // NB: failed at using std::reverse_copy() for this,
                // so writing it manually
                while(start != old_end) {
                    --new_end;
                    --old_end;
                    *new_end = *old_end;
                }
            }

            // Step 3: insert new values at gap
            {
                for(auto a = begin() + p, b = begin() + p + n; a != b; ++a) {
                    *a = val;
                }

                return begin() + p;
            }
        }

        template <class InputIterator>
        inline iterator insert(const_iterator position, InputIterator first, InputIterator last) {
            // TODO: Optimize for random access iterators (multiple insertions at once)

            // Remember integer offset before iterator invalidation
            const auto p = (position - cbegin());
            auto pi = p;

            for(; first != last; ++first) {
                insert(cbegin() + pi, value_type(*first));
                pi++;
            }

            return begin() + p;
        }

        inline iterator insert(const_iterator position, value_type&& val) {
            const auto& v = val;
            return insert(position, v);
        }

        inline iterator insert(const_iterator position, std::initializer_list<value_type> il) {
            return insert(position, il.begin(), il.end());
        }

        inline iterator erase(const_iterator position) {
            return erase(position, position + 1);
        }

        inline iterator erase(const_iterator first, const_iterator last) {
            auto from = (first - cbegin());
            auto to = (last - cbegin());
            auto n = to - from;
            std::copy(begin() + to, end(), begin() + from);

            this->m_real_size -= n;

            auto obsolete_backing = this->m_vec.size() - bits2backing(elem2bits(this->m_real_size));
            this->m_vec.erase(this->m_vec.cend() - obsolete_backing, this->m_vec.cend());
            return begin() + from;
        }

        inline void swap(BitPackingVector& other) {
            this->m_vec.swap(other.m_vec);
            std::swap(this->m_real_size, other.m_real_size);
            auto a = this->width();
            auto b = other.width();
            this->set_width_raw(b);
            other.set_width_raw(a);
        }

        inline void clear() {
            this->m_vec.clear();
            this->m_real_size = 0;
        }

        template <class... Args>
        inline iterator emplace(const_iterator position, Args&&... args) {
            return insert(position, value_type(std::forward<Args...>(args)...));
        }

        template <class... Args>
        inline void emplace_back(Args&&... args) {
            push_back(value_type(std::forward<Args...>(args)...));
        }

        inline uint8_t width() const {
            return this->raw_width();
        }

        inline void width(uint8_t w) {
            this->bit_reserve(this->size() * w);
            this->resize(this->size(), 0, w);
        }

        inline void resize(size_type n, const value_type& val, uint8_t w) {
            auto old_width = this->width();
            auto new_width = w;
            auto old_size = this->size();
            auto new_size = n;

            auto new_bit_size = elem2bits_w(new_size, new_width);
            auto common_size = std::min(old_size, new_size);

            if (old_width < new_width) {
                // grow

                // Read from position of last element in the old width grid,
                // and write to position of last element in the new width grid
                auto old_p = bitpos2backingpos_w(elem2bits_w(common_size, old_width));
                auto new_p = bitpos2backingpos_w(elem2bits_w(common_size, new_width));

                // make room for new bits, reallocating as needed
                this->m_vec.resize(bits2backing_w(new_bit_size));
                this->set_width_raw(w);
                this->m_real_size = new_size;

                uint64_t* old_ptr = this->m_vec.data() + old_p.pos;
                uint64_t* new_ptr = this->m_vec.data() + new_p.pos;

                // move elements into new width grid
                for (uint64_t i = 0; i < common_size; i++) {
                    sdsl::bits::move_left((const uint64_t*&) old_ptr, old_p.offset, old_width);
                    auto v = sdsl::bits::read_int(           old_ptr, old_p.offset, old_width);

                    sdsl::bits::move_left((const uint64_t*&) new_ptr,    new_p.offset, new_width);
                    sdsl::bits::write_int(                   new_ptr, v, new_p.offset, new_width);
                }
            } else if (old_width > new_width) {
                // shrink

                uint64_t* old_ptr = this->m_vec.data();
                uint64_t* new_ptr = this->m_vec.data();

                uint8_t old_offset = 0;
                uint8_t new_offset = 0;

                // move elements into new width grid
                for (uint64_t i = 0; i < common_size; i++) {
                    auto v = sdsl::bits::read_int_and_move((const uint64_t*&) old_ptr, old_offset, old_width);
                    sdsl::bits::write_int_and_move(new_ptr, v, new_offset, new_width);
                }

                // remove extra bits, dropping as needed
                this->m_vec.resize(bits2backing_w(new_bit_size));
                this->set_width_raw(w);
                this->m_real_size = new_size;
            }

            // initialize new elements correctly
            if (old_size < new_size) {
                auto a = this->begin() + old_size;
                auto b = this->end();
                for(; a != b; ++a) {
                    *a = val;
                }
            }
        }

        inline void bit_reserve(uint64_t n) {
            this->m_vec.reserve(bits2backing(n));
        }

        inline void reserve(uint64_t n, uint8_t w) {
            this->bit_reserve(n * w);
        }
    };

    template<class N>
    bool operator==(const BitPackingVector<N>& lhs, const BitPackingVector<N>& rhs) {
        DCHECK(lhs.width() == rhs.width());
        if (lhs.size() == rhs.size()) {
            auto extra_bits = lhs.backing2bits(lhs.m_vec.size())
                              - lhs.elem2bits(lhs.m_real_size);

            if (extra_bits == 0) {
                return lhs.m_vec == rhs.m_vec;
            } else {
                if (!std::equal(lhs.m_vec.cbegin(), lhs.m_vec.cend() - 1, rhs.m_vec.cbegin())) {
                    return false;
                }
                auto occupied_bits = lhs.backing2bits(1) - extra_bits;
                // NB: Underflow can not happen here because there are never
                // completely unoccupied backing integers
                auto elements = (occupied_bits - 1) / lhs.elem2bits(1) + 1;

                return std::equal(lhs.cend() - elements, lhs.cend(), rhs.cend() - elements);
            }
        }
        return false;
    }

    template<class N>
    bool operator!=(const BitPackingVector<N>& lhs, const BitPackingVector<N>& rhs) {
        return !(lhs == rhs);
    }

    template<class N>
    bool operator<(const BitPackingVector<N>& lhs, const BitPackingVector<N>& rhs) {
        return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
    }

    template<class N>
    bool operator<=(const BitPackingVector<N>& lhs, const BitPackingVector<N>& rhs) {
        return !(lhs > rhs);
    }

    template<class N>
    bool operator>(const BitPackingVector<N>& lhs, const BitPackingVector<N>& rhs) {
        return std::lexicographical_compare(rhs.cbegin(), rhs.cend(), lhs.cbegin(), lhs.cend());
    }

    template<class N>
    bool operator>=(const BitPackingVector<N>& lhs, const BitPackingVector<N>& rhs) {
        return !(lhs < rhs);
    }

    template<class N>
    void swap(BitPackingVector<N>& x, BitPackingVector<N>& y) {
        x.swap(y);
    }
}

template<class T>
using BitPackingVector = int_vector::BitPackingVector<T>;

}

/// \endcond

