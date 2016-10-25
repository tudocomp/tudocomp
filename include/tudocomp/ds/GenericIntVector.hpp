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

#include <tudocomp/ds/uint_t.hpp>
#include <tudocomp/util/IntegerBase.hpp>

#include <sdsl/bits.hpp>
#include <glog/logging.h>

namespace tdc {
namespace int_vector {
    using sdsl::bits;

    typedef uint64_t DynamicIntValueType;

    // TODO: On rrferences, provide 32 or 64 bit ops depending on bits
    // of uint_t type

    struct Const {
        typedef const DynamicIntValueType* ptr;
        typedef uint8_t                    off;
        typedef const uint8_t              len;
        typedef DynamicIntValueType        val;
    };

    struct Mut {
        typedef DynamicIntValueType* ptr;
        typedef uint8_t              off;
        typedef const uint8_t        len;
        typedef DynamicIntValueType  val;
    };

    template<class T, class U, class V>
    class GenericIntRef;
    class IntRef;
    class ConstIntRef;
    class IntPtr;
    class ConstIntPtr;

    template<class Self, class T>
    class GenericIntPtr {
    protected:
        friend class GenericIntRef<IntRef, IntPtr, Mut>;
        friend class GenericIntRef<ConstIntRef, ConstIntPtr, Const>;
        friend class IntRef;
        friend class ConstIntRef;
        friend class IntegerBaseTrait<IntRef>;
        friend class IntegerBaseTraitConst<IntRef>;
        friend class IntegerBaseTraitConst<ConstIntRef>;

        typename T::ptr m_ptr;
        typename T::off m_bit_offset;
        typename T::len m_bit_size;
    public:
        GenericIntPtr(typename T::ptr ptr,
                      typename T::off bit_offset,
                      typename T::len bit_size):
            m_ptr(ptr),
            m_bit_offset(bit_offset),
            m_bit_size(bit_size) {}
        GenericIntPtr(): GenericIntPtr("\0\0\0\0", 0, 0) {}
        GenericIntPtr(const GenericIntPtr& other):
            GenericIntPtr(other.m_ptr, other.m_bit_offset, other.m_bit_size) {}

        Self& operator=(const Self& other) {
            DCHECK(m_bit_size == other.m_bit_size);
            m_ptr = other.m_ptr;
            m_bit_offset = other.m_bit_offset;
            return static_cast<Self&>(*this);
        }

        Self& operator++() {
            const DynamicIntValueType* tmp = m_ptr;
            bits::move_right(tmp, m_bit_offset, m_bit_size);
            m_ptr = (DynamicIntValueType*) tmp;
            return static_cast<Self&>(*this);
        }

        Self& operator--() {
            const DynamicIntValueType* tmp = m_ptr;
            bits::move_left(tmp, m_bit_offset, m_bit_size);
            m_ptr = (DynamicIntValueType*) tmp;
            return static_cast<Self&>(*this);
        }

        friend Self operator+(const Self& lhs, const ptrdiff_t& rhs) {
            auto tmp = lhs;
            if (rhs >= 0) {
                for(size_t i = 0; i < size_t(rhs); i++) {
                    ++tmp;
                }
            } else {
                for(size_t i = 0; i < size_t(-rhs); i++) {
                    --tmp;
                }
            }
            return tmp;
        }

        friend Self operator+(const ptrdiff_t& lhs, const Self& rhs) { return rhs + lhs;      }
        friend Self operator-(const Self& lhs, const ptrdiff_t& rhs) { return lhs + (-rhs);   }

        friend ptrdiff_t operator-(const Self& lhs, const Self& rhs) {
            // TODO: test
            DCHECK(lhs.m_bit_size == rhs.m_bit_size);
            auto ptr_diff = lhs.m_ptr - rhs.m_ptr;
            auto bit_count = ptr_diff * sizeof(typename T::val) * CHAR_BIT;
            bit_count += lhs.m_bit_offset;
            bit_count -= rhs.m_bit_offset;
            bit_count /= lhs.m_bit_size;
            return bit_count;
        }

        friend bool operator==(const Self& lhs, const Self& rhs) {
            DCHECK(lhs.m_bit_size == rhs.m_bit_size);
            return (lhs.m_ptr == rhs.m_ptr)
                && (lhs.m_bit_offset == rhs.m_bit_offset);
        }
        friend bool operator<(const Self& lhs, const Self& rhs)  {
            // TODO: test
            DCHECK(lhs.m_bit_size == rhs.m_bit_size);
            if (lhs.m_ptr < rhs.m_ptr) return true;
            if ((lhs.m_ptr == rhs.m_ptr) && (lhs.m_bit_offset < rhs.m_bit_offset)) return true;
            return false;
        }
        friend bool operator!=(const Self& lhs, const Self& rhs) { return !(lhs == rhs);               }
        friend bool operator>(const Self& lhs, const Self& rhs)  { return !(lhs < rhs);                }
        friend bool operator>=(const Self& lhs, const Self& rhs) { return (lhs > rhs) || (lhs == rhs); }
        friend bool operator<=(const Self& lhs, const Self& rhs) { return (lhs < rhs) || (lhs == rhs); }

        Self& operator+=(const ptrdiff_t& v) { auto& self = static_cast<Self&>(*this); self = (self + v); return self; }
        Self& operator-=(const ptrdiff_t& v) { auto& self = static_cast<Self&>(*this); self = (self - v); return self; }

        Self operator++(int) { auto self = static_cast<Self&>(*this); ++(*this); return self; }
        Self operator--(int) { auto self = static_cast<Self&>(*this); --(*this); return self; }

        ConstIntRef operator[](size_t i) const;
        ConstIntRef operator*() const;
    };

    class ConstIntPtr: public GenericIntPtr<ConstIntPtr, Const> {
    public:
        using GenericIntPtr<ConstIntPtr, Const>::GenericIntPtr;
        inline ConstIntPtr(const ConstIntPtr& other): GenericIntPtr(other) {}
        ConstIntPtr& operator=(const ConstIntPtr& other) {
            return ((GenericIntPtr<ConstIntPtr, Const>&) *this) = other;
        }
    };

    class IntPtr: public GenericIntPtr<IntPtr, Mut> {
    public:
        using GenericIntPtr<IntPtr, Mut>::GenericIntPtr;
        inline IntPtr(const IntPtr& other): GenericIntPtr(other) {}
        IntPtr& operator=(const IntPtr& other) {
            return ((GenericIntPtr<IntPtr, Mut>&) *this) = other;
        }

        inline IntRef operator*();
        inline IntRef operator[](size_t i);
    };

    template<class Self, class Ptr, class T>
    class GenericIntRef {
    protected:
        friend class IntPtr;
        friend class ConstIntPtr;
        friend class IntegerBaseTrait<IntRef>;
        friend class IntegerBaseTraitConst<IntRef>;
        friend class IntegerBaseTraitConst<ConstIntRef>;

        Ptr m_ptr;
    public:
        typedef typename T::val value_type;

        explicit GenericIntRef(const Ptr& ptr): m_ptr(ptr) {}

        operator value_type() const {
            return bits::read_int(m_ptr.m_ptr, m_ptr.m_bit_offset, m_ptr.m_bit_size);
        }

    };

    class IntRef: public GenericIntRef<IntRef, IntPtr, Mut> {
    public:
        explicit IntRef(const IntPtr& ptr): GenericIntRef(ptr) {}

        IntRef& operator=(value_type other)
        {
            bits::write_int(m_ptr.m_ptr, other, m_ptr.m_bit_offset, m_ptr.m_bit_size);
            return *this;
        };

    };

    class ConstIntRef: public GenericIntRef<ConstIntRef, ConstIntPtr, Const> {
    public:
        explicit ConstIntRef(const ConstIntPtr& ptr): GenericIntRef(ptr) {}
    };


    inline IntRef IntPtr::operator*() {
        return IntRef(*this);
    }

    template<class Self, class U>
    ConstIntRef GenericIntPtr<Self, U>::operator*() const {
        return ConstIntRef(static_cast<const Self&>(*this));
    }

    template<class Self, class U>
    ConstIntRef GenericIntPtr<Self, U>::operator[](size_t i) const {
        auto x = *this;
        x += i;
        return ConstIntRef(static_cast<const Self&>(x));
    }

    inline IntRef IntPtr::operator[](size_t i) {
        auto x = *this;
        x += i;
        return IntRef(x);
    }

    static_assert(sizeof(GenericIntPtr<ConstIntPtr, Const>) <= (sizeof(void*) * 2), "make sure this is reasonably small");

    template<class T, class X = void>
    class GenericIntVectorTrait {
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
    class GenericIntVectorTrait<uint_t<N>, typename std::enable_if<(N % 8) == 0>::type> {
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
    class GenericIntVectorTrait<uint_t<N>, typename std::enable_if<(N % 8) != 0>::type> {
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
    class GenericIntVector {
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
    };

}

template<>
struct IntegerBaseTraitConst<int_vector::IntRef> {
    typedef sdsl::bits bits;
    typedef int_vector::IntRef T;

    typedef uint64_t SelfMaxBit;

    inline static SelfMaxBit cast_for_self_op(const T& self) {
        return bits::read_int(self.m_ptr.m_ptr,
                              self.m_ptr.m_bit_offset,
                              self.m_ptr.m_bit_size);
    }
    inline static SelfMaxBit cast_for_32_op(const T& self) {
        return bits::read_int(self.m_ptr.m_ptr,
                              self.m_ptr.m_bit_offset,
                              self.m_ptr.m_bit_size);

    }
    inline static uint64_t cast_for_64_op(const T& self) {
        return bits::read_int(self.m_ptr.m_ptr,
                              self.m_ptr.m_bit_offset,
                              self.m_ptr.m_bit_size);
    }
};

template<>
struct IntegerBaseTrait<int_vector::IntRef> {
    typedef sdsl::bits bits;
    typedef int_vector::IntRef T;

    inline static void assign(T& self, uint32_t v) {
        bits::write_int(self.m_ptr.m_ptr,
                        v,
                        self.m_ptr.m_bit_offset,
                        self.m_ptr.m_bit_size);
    }
    inline static void assign(T& self, uint64_t v) {
        bits::write_int(self.m_ptr.m_ptr,
                        v,
                        self.m_ptr.m_bit_offset,
                        self.m_ptr.m_bit_size);
    }
};

template<>
struct IntegerBaseTraitConst<int_vector::ConstIntRef> {
    typedef sdsl::bits bits;
    typedef int_vector::ConstIntRef T;

    typedef uint64_t SelfMaxBit;

    inline static SelfMaxBit cast_for_self_op(const T& self) {
        return bits::read_int(self.m_ptr.m_ptr,
                              self.m_ptr.m_bit_offset,
                              self.m_ptr.m_bit_size);
    }
    inline static SelfMaxBit cast_for_32_op(const T& self) {
        return bits::read_int(self.m_ptr.m_ptr,
                              self.m_ptr.m_bit_offset,
                              self.m_ptr.m_bit_size);

    }
    inline static uint64_t cast_for_64_op(const T& self) {
        return bits::read_int(self.m_ptr.m_ptr,
                              self.m_ptr.m_bit_offset,
                              self.m_ptr.m_bit_size);
    }
};


}

#endif
