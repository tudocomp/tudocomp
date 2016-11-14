#ifndef TUDOCOMP_GENERIC_INT_VECTOR_H
#define TUDOCOMP_GENERIC_INT_VECTOR_H

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

#include <tudocomp/ds/GenericIntPtr.hpp>
#include <tudocomp/ds/BitPackingVector.hpp>
#include <tudocomp/ds/uint_t.hpp>
#include <tudocomp/ds/dynamic_t.hpp>
#include <tudocomp/util/IntegerBase.hpp>

#include <sdsl/bits.hpp>
#include <glog/logging.h>

namespace tdc {
namespace int_vector {
    /*
     * TODO:
     o constructor for int width
     o swap/reassign ops
     o bit capacity?
     o width setter
     ? void bit_resize(const size_type size);
     ? value_type get_int(size_type idx, const uint8_t len=64) const;
     ? void set_int(size_type idx, value_type x, const uint8_t len=64);
     - in-place width setting?
     - flip?

     maybe split up into raw-bits-vector and element-vector?
     */

    template<class T, class X = void>
    struct GenericIntVectorTrait {
        typedef typename std::vector<T>::value_type             value_type;

        typedef typename std::vector<T>::reference              reference;
        typedef typename std::vector<T>::const_reference        const_reference;

        typedef typename std::vector<T>::pointer                pointer;
        typedef typename std::vector<T>::const_pointer          const_pointer;

        typedef typename std::vector<T>::iterator               iterator;
        typedef typename std::vector<T>::const_iterator         const_iterator;

        typedef typename std::vector<T>::reverse_iterator       reverse_iterator;
        typedef typename std::vector<T>::const_reverse_iterator const_reverse_iterator;

        typedef typename std::vector<T>::difference_type        difference_type;
        typedef typename std::vector<T>::size_type              size_type;

        typedef          std::vector<T>                         backing_data;
        typedef          T                                      internal_data_type;

        inline static uint64_t bit_size(const backing_data& self) {
            return sizeof(T) * CHAR_BIT * self.size();
        }

        inline static uint64_t bit_capacity(const backing_data& self) {
            return sizeof(T) * CHAR_BIT * self.capacity();
        }

        static constexpr ElementStorageMode element_storage_mode() {
            return ElementStorageMode::Direct;
        }

        inline static uint8_t width(const backing_data& self) {
            return sizeof(T) * CHAR_BIT;
        }

        inline static backing_data with_width(size_type n, const value_type& val, uint8_t width) {
            width_error();
            return backing_data(n, val);
        }

        inline static void width(backing_data& self, uint8_t w) {
            width_error();
        }

        inline static void resize(backing_data& self, size_type n, const value_type& val, uint8_t w) {
            width_error();
        }

        inline static void bit_reserve(backing_data& self, uint64_t n) {
            width_error();
        }
    };

    template<>
    struct GenericIntVectorTrait<dynamic_t> {
        typedef typename BitPackingVector<dynamic_t>::value_type             value_type;

        typedef typename BitPackingVector<dynamic_t>::reference              reference;
        typedef typename BitPackingVector<dynamic_t>::const_reference        const_reference;

        typedef typename BitPackingVector<dynamic_t>::pointer                pointer;
        typedef typename BitPackingVector<dynamic_t>::const_pointer          const_pointer;

        typedef typename BitPackingVector<dynamic_t>::iterator               iterator;
        typedef typename BitPackingVector<dynamic_t>::const_iterator         const_iterator;

        typedef typename BitPackingVector<dynamic_t>::reverse_iterator       reverse_iterator;
        typedef typename BitPackingVector<dynamic_t>::const_reverse_iterator const_reverse_iterator;

        typedef typename BitPackingVector<dynamic_t>::difference_type        difference_type;
        typedef typename BitPackingVector<dynamic_t>::size_type              size_type;

        typedef          BitPackingVector<dynamic_t>                         backing_data;
        typedef typename BitPackingVector<dynamic_t>::internal_data_type     internal_data_type;

        inline static uint64_t bit_size(const backing_data& self) {
            return self.size() * self.width();
        }

        inline static uint64_t bit_capacity(const backing_data& self) {
            return self.capacity() * self.width();
        }

        static constexpr ElementStorageMode element_storage_mode() {
            return ElementStorageMode::BitPacked;
        }

        inline static uint8_t width(const backing_data& self) {
            return self.width();
        }

        inline static backing_data with_width(size_type n, const value_type& val, uint8_t width) {
            return backing_data(n, val, width);
        }

        inline static void width(backing_data& self, uint8_t w) {
            self.width(w);
        }

        inline static void resize(backing_data& self, size_type n, const value_type& val, uint8_t w) {
            self.resize(n, val, w);
        }

        inline static void bit_reserve(backing_data& self, uint64_t n) {
            self.bit_reserve(n);
        }
    };

    template<size_t N>
    struct GenericIntVectorTrait<uint_t<N>, typename std::enable_if<(N % 8) != 0>::type> {
        typedef typename BitPackingVector<uint_t<N>>::value_type             value_type;

        typedef typename BitPackingVector<uint_t<N>>::reference              reference;
        typedef typename BitPackingVector<uint_t<N>>::const_reference        const_reference;

        typedef typename BitPackingVector<uint_t<N>>::pointer                pointer;
        typedef typename BitPackingVector<uint_t<N>>::const_pointer          const_pointer;

        typedef typename BitPackingVector<uint_t<N>>::iterator               iterator;
        typedef typename BitPackingVector<uint_t<N>>::const_iterator         const_iterator;

        typedef typename BitPackingVector<uint_t<N>>::reverse_iterator       reverse_iterator;
        typedef typename BitPackingVector<uint_t<N>>::const_reverse_iterator const_reverse_iterator;

        typedef typename BitPackingVector<uint_t<N>>::difference_type        difference_type;
        typedef typename BitPackingVector<uint_t<N>>::size_type              size_type;

        typedef          BitPackingVector<uint_t<N>>                         backing_data;
        typedef typename BitPackingVector<uint_t<N>>::internal_data_type     internal_data_type;

        inline static uint64_t bit_size(const backing_data& self) {
            return self.size() * N;
        }

        inline static uint64_t bit_capacity(const backing_data& self) {
            return self.capacity() * N;
        }

        static constexpr ElementStorageMode element_storage_mode() {
            return ElementStorageMode::BitPacked;
        }

        inline static uint8_t width(const backing_data& self) {
            return self.width();
        }

        inline static backing_data with_width(size_type n, const value_type& val, uint8_t width) {
            width_error();
            return backing_data(n, val);
        }

        inline static void width(backing_data& self, uint8_t w) {
            width_error();
        }

        inline static void resize(backing_data& self, size_type n, const value_type& val, uint8_t w) {
            width_error();
        }

        inline static void bit_reserve(backing_data& self, uint64_t n) {
            width_error();
        }
    };

    template<class T>
    class GenericIntVector {
        // TODO: Add custom allocator support
    public:
        typedef typename GenericIntVectorTrait<T>::value_type             value_type;
        typedef typename GenericIntVectorTrait<T>::reference              reference;
        typedef typename GenericIntVectorTrait<T>::const_reference        const_reference;
        typedef typename GenericIntVectorTrait<T>::pointer                pointer;
        typedef typename GenericIntVectorTrait<T>::const_pointer          const_pointer;
        typedef typename GenericIntVectorTrait<T>::iterator               iterator;
        typedef typename GenericIntVectorTrait<T>::const_iterator         const_iterator;
        typedef typename GenericIntVectorTrait<T>::reverse_iterator       reverse_iterator;
        typedef typename GenericIntVectorTrait<T>::const_reverse_iterator const_reverse_iterator;
        typedef typename GenericIntVectorTrait<T>::difference_type        difference_type;
        typedef typename GenericIntVectorTrait<T>::size_type              size_type;

        /// The element type of the internal data buffer accessed with data()
        typedef typename GenericIntVectorTrait<T>::internal_data_type     internal_data_type;

        static constexpr ElementStorageMode element_storage_mode() {
            return GenericIntVectorTrait<T>::element_storage_mode();
        }
    private:
        typename GenericIntVectorTrait<T>::backing_data m_data;
    public:
        // default
        inline explicit GenericIntVector() {}

        // fill
        explicit GenericIntVector(size_type n): m_data(n) {}
        inline GenericIntVector(size_type n, const value_type& val): m_data(n, val) {}
        inline GenericIntVector(size_type n, const value_type& val, uint8_t width):
            m_data(GenericIntVectorTrait<T>::with_width(n, val, width)) {}

        // range
        template <class InputIterator>
        inline GenericIntVector(InputIterator first, InputIterator last): m_data(first, last) {}

        // copy
        inline GenericIntVector(const GenericIntVector& other): m_data(other.m_data) {}

        // move
        inline GenericIntVector(GenericIntVector&& other): m_data(std::move(other.m_data)) {}

        // initializer list
        inline GenericIntVector(std::initializer_list<value_type> il): m_data(il) {}

        inline GenericIntVector& operator=(const GenericIntVector& other) {
            m_data = other.m_data;
            return *this;
        }

        inline GenericIntVector& operator=(GenericIntVector&& other) {
            m_data = std::move(other.m_data);
            return *this;
        }

        inline GenericIntVector& operator=(std::initializer_list<value_type> il) {
            m_data = il;
            return *this;
        }

        inline iterator begin() {
            return m_data.begin();
        }

        inline iterator end() {
            return m_data.end();
        }

        inline reverse_iterator rbegin() {
            return m_data.rbegin();
        }

        inline reverse_iterator rend() {
            return m_data.rend();
        }

        inline const_iterator begin() const {
            return m_data.begin();
        }

        inline const_iterator end() const {
            return m_data.end();
        }

        inline const_reverse_iterator rbegin() const {
            return m_data.rbegin();
        }

        inline const_reverse_iterator rend() const {
            return m_data.rend();
        }

        inline const_iterator cbegin() const {
            return m_data.cbegin();
        }

        inline const_iterator cend() const {
            return m_data.cend();
        }

        inline const_reverse_iterator crbegin() const {
            return m_data.crbegin();
        }

        inline const_reverse_iterator crend() const {
            return m_data.crend();
        }

        inline size_type size() const {
            return m_data.size();
        }

        inline uint64_t bit_size() const {
            return GenericIntVectorTrait<T>::bit_size(m_data);
        }

        inline size_type max_size() const {
            return m_data.max_size();
        }

        inline uint8_t width() const {
            return GenericIntVectorTrait<T>::width(m_data);
        }

        inline void width(uint8_t w) {
            GenericIntVectorTrait<T>::width(m_data, w);
        }

        inline void resize(size_type n) {
            m_data.resize(n);
        }

        inline void resize(size_type n, const value_type& val) {
            m_data.resize(n, val);
        }

        inline void resize(size_type n, const value_type& val, uint8_t w) {
            GenericIntVectorTrait<T>::resize(m_data, n, val, w);
        }

        inline size_type capacity() const {
            return m_data.capacity();
        }

        inline uint64_t bit_capacity() const {
            return GenericIntVectorTrait<T>::bit_capacity(m_data);
        }

        inline bool empty() const {
            return m_data.empty();
        }

        inline void reserve(size_type n) {
            m_data.reserve(n);
        }

        inline void bit_reserve(uint64_t n) {
            GenericIntVectorTrait<T>::bit_reserve(m_data, n);
        }

        inline void shrink_to_fit() {
            m_data.shrink_to_fit();
        }

        inline reference operator[](size_type n) {
            return m_data[n];
        }

        inline const_reference operator[](size_type n) const {
            return m_data[n];
        }

        inline reference at(size_type n) {
            return m_data.at(n);
        }

        inline const_reference at(size_type n) const {
            return m_data.at(n);
        }

        inline reference front() {
            return m_data.front();
        }

        inline const_reference front() const {
            return m_data.front();
        }

        inline reference back() {
            return m_data.back();
        }

        inline const_reference back() const {
            return m_data.back();
        }

        inline internal_data_type* data() noexcept {
            return m_data.data();
        }

        inline const internal_data_type* data() const noexcept {
            return m_data.data();
        }

        template <class InputIterator>
        inline void assign(InputIterator first, InputIterator last) {
            m_data.assign(first, last);
        }

        inline void assign(size_type n, const value_type& val) {
            m_data.assign(n, val);
        }

        inline void assign(std::initializer_list<value_type> il) {
            m_data.assign(il);
        }

        inline void push_back(const value_type& val) {
            m_data.push_back(val);
        }

        inline void push_back(value_type&& val) {
            m_data.push_back(std::move(val));
        }

        inline void pop_back() {
            m_data.pop_back();
        }

        inline iterator insert(const_iterator position, const value_type& val) {
            return m_data.insert(position, val);
        }

        inline iterator insert(const_iterator position, size_type n, const value_type& val) {
            return m_data.insert(position, n, val);
        }

        template <class InputIterator>
        inline iterator insert(const_iterator position, InputIterator first, InputIterator last) {
            return m_data.insert(position, first, last);
        }

        inline iterator insert(const_iterator position, value_type&& val) {
            return m_data.insert(position, std::move(val));
        }

        inline iterator insert(const_iterator position, std::initializer_list<value_type> il) {
            return m_data.insert(position, il);
        }

        inline iterator erase(const_iterator position) {
            return m_data.erase(position);
        }

        inline iterator erase(const_iterator first, const_iterator last) {
            return m_data.erase(first, last);
        }

        inline void swap(GenericIntVector& other) {
            m_data.swap(other.m_data);
        }

        inline void clear() {
            m_data.clear();
        }

        template <class... Args>
        inline iterator emplace(const_iterator position, Args&&... args) {
            return m_data.emplace(position, std::forward<Args...>(args)...);
        }

        template <class... Args>
        inline void emplace_back(Args&&... args) {
            m_data.emplace_back(std::forward<Args...>(args)...);
        }

        template<class U>
        friend bool operator==(const GenericIntVector<U>& lhs, const GenericIntVector<U>& rhs);
        template<class U>
        friend bool operator!=(const GenericIntVector<U>& lhs, const GenericIntVector<U>& rhs);
        template<class U>
        friend bool operator<(const GenericIntVector<U>& lhs, const GenericIntVector<U>& rhs);
        template<class U>
        friend bool operator<=(const GenericIntVector<U>& lhs, const GenericIntVector<U>& rhs);
        template<class U>
        friend bool operator>(const GenericIntVector<U>& lhs, const GenericIntVector<U>& rhs);
        template<class U>
        friend bool operator>=(const GenericIntVector<U>& lhs, const GenericIntVector<U>& rhs);
        template<class U>
        friend void swap(GenericIntVector<U>& lhs, GenericIntVector<U>& rhs);
    };

    template<class T>
    bool operator==(const GenericIntVector<T>& lhs, const GenericIntVector<T>& rhs) {
        return lhs.m_data == rhs.m_data;
    }

    template<class T>
    bool operator!=(const GenericIntVector<T>& lhs, const GenericIntVector<T>& rhs) {
        return lhs.m_data != rhs.m_data;
    }

    template<class T>
    bool operator<(const GenericIntVector<T>& lhs, const GenericIntVector<T>& rhs) {
        return lhs.m_data < rhs.m_data;
    }

    template<class T>
    bool operator<=(const GenericIntVector<T>& lhs, const GenericIntVector<T>& rhs) {
        return lhs.m_data <= rhs.m_data;
    }

    template<class T>
    bool operator>(const GenericIntVector<T>& lhs, const GenericIntVector<T>& rhs) {
        return lhs.m_data > rhs.m_data;
    }

    template<class T>
    bool operator>=(const GenericIntVector<T>& lhs, const GenericIntVector<T>& rhs) {
        return lhs.m_data >= rhs.m_data;
    }

    template<class T>
    void swap(GenericIntVector<T>& lhs, GenericIntVector<T>& rhs) {
        using std::swap;
        swap(lhs.m_data, rhs.m_data);
    }

}

}

#endif
