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

#include <tudocomp/ds/uint_t.hpp>

#include <sdsl/bits.hpp>
#include <glog/logging.h>

namespace tdc {
namespace int_vector {
    using sdsl::bits;

    typedef uint64_t DynamicIntValueType;

    struct Const {
        typedef const DynamicIntValueType* ptr;
        typedef const uint16_t             off;
        typedef const uint16_t             len;
        typedef DynamicIntValueType        val;
    };

    struct Mut {
        typedef DynamicIntValueType* ptr;
        typedef uint16_t             off;
        typedef uint16_t             len;
        typedef DynamicIntValueType  val;
    };

    template<class T>
    class DynamicIntRef;
    class IntRef;
    class ConstIntRef;

    template<class T>
    class DynamicIntPtr {
    protected:
        friend class DynamicIntRef<Const>;
        friend class DynamicIntRef<Mut>;
        friend class IntRef;
        friend class ConstIntRef;

        typename T::ptr m_ptr;
        typename T::off m_bit_offset;
        typename T::len m_bit_size;
    public:
        DynamicIntPtr(typename T::ptr ptr,
                      typename T::off bit_offset,
                      typename T::len bit_size):
            m_ptr(ptr),
            m_bit_offset(bit_offset),
            m_bit_size(bit_size) {}

        // TODO: Big bug, ensure the operators return the right child class

        DynamicIntPtr(): DynamicIntPtr("\0\0\0\0", 0, 0) {}

        DynamicIntPtr(const DynamicIntPtr<T>& other):
            DynamicIntPtr(other.m_ptr, other.m_bit_offset, other.m_bit_size) {}

        DynamicIntPtr<T>& operator=(const DynamicIntPtr<T>& other) {
            m_ptr = other.m_ptr;
            m_bit_offset = other.m_bit_offset;
            m_bit_size = other.m_bit_size;
        }

        bool operator==(const DynamicIntPtr<T>& other) {
            return (m_ptr == other.m_ptr)
                && (m_bit_offset == other.m_bit_offset)
                && (m_bit_size == other.m_bit_size);
        }

        bool operator!=(const DynamicIntPtr<T>& other) {
            return !(*this == other);
        }

        T::val operator->() {
            return **this;
        }

        DynamicIntPtr<T> operator++(int) {
            auto x = *this;
            ++(*this);
            return x;
        }

        DynamicIntPtr<T>& operator++() {
            bits::move_right(m_ptr, m_bit_offset, m_bit_size);
            return *this;
        }

        DynamicIntPtr<T> operator--(int) {
            auto x = *this;
            --(*this);
            return x;
        }

        DynamicIntPtr<T>& operator--() {
            bits::move_left(m_ptr, m_bit_offset, m_bit_size);
            return *this;
        }

        // TODO: Make sure these operations don't end up O(N)

        DynamicIntPtr<T>& operator+=(size_t i) {
            for(size_t j = 0; j < i; j++) {
                ++(*this);
            }
            return *this;
        }

        DynamicIntPtr<T>& operator-=(size_t i) {
            for(size_t j = 0; j < i; j++) {
                --(*this);
            }
            return *this;
        }

        ConstIntRef operator[](size_t i) const {
            auto x = *this;
            x += i;
            return ConstIntRef(x);
        }

    };

    DynamicIntPtr<T> operator+(V, DynamicIntPtr<T>) {

    }

    DynamicIntPtr<T> operator+(DynamicIntPtr<T>, V) {

    }

    DynamicIntPtr<T> operator-(DynamicIntPtr<T>, V) {

    }

    size_t operator-(DynamicIntPtr<T>, DynamicIntPtr<T>) {

    }

    bool operator<(DynamicIntPtr<T>, DynamicIntPtr<T>) {

    }

    bool operator>(DynamicIntPtr<T>, DynamicIntPtr<T>) {

    }

    bool operator<=(DynamicIntPtr<T>, DynamicIntPtr<T>) {

    }

    bool operator>=(DynamicIntPtr<T>, DynamicIntPtr<T>) {

    }

    void swap(DynamicIntPtr<T>&, DynamicIntPtr<T>&)
    {
        printf("swap(A, A)\n");
    }

    static_assert(sizeof(DynamicIntPtr<Const>) <= (sizeof(void*) * 2), "make sure this is reasonably small");

    template<class T>
    class DynamicIntRef {
    protected:
        friend class DynamicIntPtr<Const>;
        friend class DynamicIntPtr<Mut>;
        friend class IntPtr;
        friend class ConstIntPtr;

        DynamicIntPtr<T> m_ptr;
    public:
        typedef typename T::val value_type;

        explicit DynamicIntRef(const DynamicIntPtr<T>& ptr): m_ptr(ptr) {}

        operator value_type() const {
            return bits::read_int(m_ptr.m_ptr, m_ptr.m_bit_offset, m_ptr.m_bit_size);
        }

        template<class U>
        bool operator==(const DynamicIntRef<U>& other) const {
            return value_type(*this) == value_type(other);
        }

        template<class U>
        bool operator<(const DynamicIntRef<U>& other) const {
            return value_type(*this) < value_type(other);
        }
    };


    class IntPtr: public DynamicIntPtr<Mut> {
    public:
        IntPtr(Mut::ptr ptr,
               Mut::off bit_offset,
               Mut::len bit_size): DynamicIntPtr(ptr, bit_offset, bit_size) {}
        inline IntRef operator*();
    };

    class ConstIntPtr: public DynamicIntPtr<Const> {
    public:
        ConstIntPtr(Const::ptr ptr,
                    Const::off bit_offset,
                    Const::len bit_size): DynamicIntPtr(ptr, bit_offset, bit_size) {}
        inline ConstIntRef operator*() const;
    };

    class IntRef: public DynamicIntRef<Mut> {
    public:
        explicit IntRef(const DynamicIntPtr<Mut>& ptr):
            DynamicIntRef(ptr) {}

        IntRef& operator=(value_type other)
        {
            bits::write_int(m_ptr.m_ptr, other, m_ptr.m_bit_offset, m_ptr.m_bit_size);
            return *this;
        };

        IntRef& operator++()
        {
            value_type x = bits::read_int(m_ptr.m_ptr, m_ptr.m_bit_offset, m_ptr.m_bit_size);
            bits::write_int(m_ptr.m_ptr, x + 1, m_ptr.m_bit_offset, m_ptr.m_bit_size);
            return *this;
        }

        value_type operator++(int)
        {
            value_type val(*this);
            ++(*this);
            return val;
        }

        IntRef& operator--()
        {
            value_type x = bits::read_int(m_ptr.m_ptr, m_ptr.m_bit_offset, m_ptr.m_bit_size);
            bits::write_int(m_ptr.m_ptr, x - 1, m_ptr.m_bit_offset, m_ptr.m_bit_size);
            return *this;
        }

        value_type operator--(int)
        {
            value_type val(*this);
            --(*this);
            return val;
        }

        IntRef& operator+=(const value_type other)
        {
            value_type x = bits::read_int(m_ptr.m_ptr, m_ptr.m_bit_offset, m_ptr.m_bit_size);
            bits::write_int(m_ptr.m_ptr, x + other, m_ptr.m_bit_offset, m_ptr.m_bit_size);
            return *this;
        }

        IntRef& operator-=(const value_type other)
        {
            value_type x = bits::read_int(m_ptr.m_ptr, m_ptr.m_bit_offset, m_ptr.m_bit_size);
            bits::write_int(m_ptr.m_ptr, x - other, m_ptr.m_bit_offset, m_ptr.m_bit_size);
            return *this;
        }
    };

    class ConstIntRef: public DynamicIntRef<Const> {
    public:
        explicit ConstIntRef(const DynamicIntPtr<Const>& ptr):
            DynamicIntRef(ptr) {}
    };


    inline IntRef IntPtr::operator*() {
        return IntRef(*this);
    }

    inline ConstIntRef ConstIntPtr::operator*() const {
        return ConstIntRef(*this);
    }

    template<class T, class X = void>
    class DynamicIntVectorTrait {
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

        typedef typename std::vector<T>                         backing_vec;
    };

    template<size_t N>
    class DynamicIntVectorTrait<uint_t<N>, typename std::enable_if<(N % 8) == 0>::type> {
        typedef typename std::vector<uint_t<N>>::value_type             value_type;

        typedef typename std::vector<uint_t<N>>::reference              reference;
        typedef typename std::vector<uint_t<N>>::const_reference        const_reference;

        typedef typename std::vector<uint_t<N>>::pointer                pointer;
        typedef typename std::vector<uint_t<N>>::const_pointer          const_pointer;

        typedef typename std::vector<uint_t<N>>::iterator               iterator;
        typedef typename std::vector<uint_t<N>>::const_iterator         const_iterator;

        typedef typename std::vector<uint_t<N>>::reverse_iterator       reverse_iterator;
        typedef typename std::vector<uint_t<N>>::const_reverse_iterator const_reverse_iterator;

        typedef typename std::vector<uint_t<N>>::difference_type        difference_type;
        typedef typename std::vector<uint_t<N>>::size_type              size_type;

        typedef typename std::vector<uint_t<N>>                         backing_vec;
    };

    template<size_t N>
    class DynamicIntVectorTrait<uint_t<N>, typename std::enable_if<(N % 8) != 0>::type> {
        typedef uint_t<N>                                            value_type;

        typedef IntRef                                               reference;
        typedef ConstIntRef                                          const_reference;

        typedef IntPtr                                               pointer;
        typedef ConstIntPtr                                          const_pointer;

        typedef pointer                                              iterator;
        typedef const_pointer                                        const_iterator;

        typedef typename std::reverse_iterator<iterator>             reverse_iterator;
        typedef typename std::reverse_iterator<const_iterator>       const_reverse_iterator;

        typedef ptrdiff_t                                            difference_type;
        typedef size_t                                               size_type;

        typedef typename std::vector<DynamicIntValueType>            backing_vec;
    };

    template<class T>
    class DynamicIntVector {
        typedef typename DynamicIntVectorTrait<T>::value_type             value_type;
        typedef typename DynamicIntVectorTrait<T>::reference              reference;
        typedef typename DynamicIntVectorTrait<T>::const_reference        const_reference;
        typedef typename DynamicIntVectorTrait<T>::pointer                pointer;
        typedef typename DynamicIntVectorTrait<T>::const_pointer          const_pointer;
        typedef typename DynamicIntVectorTrait<T>::iterator               iterator;
        typedef typename DynamicIntVectorTrait<T>::const_iterator         const_iterator;
        typedef typename DynamicIntVectorTrait<T>::reverse_iterator       reverse_iterator;
        typedef typename DynamicIntVectorTrait<T>::const_reverse_iterator const_reverse_iterator;
        typedef typename DynamicIntVectorTrait<T>::difference_type        difference_type;
        typedef typename DynamicIntVectorTrait<T>::size_type              size_type;
    };

}
}

#endif
