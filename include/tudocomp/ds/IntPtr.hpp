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

#include <tudocomp/ds/IntRepr.hpp>
#include <tudocomp/ds/uint_t.hpp>
#include <tudocomp/ds/dynamic_t.hpp>
#include <tudocomp/util/IntegerBase.hpp>

#include <sdsl/bits.hpp>
#include <glog/logging.h>

/// \cond INTERNAL

namespace tdc {
    namespace int_vector {
        template<class T>
        class IntRef;
        template<class T>
        class ConstIntRef;
        template<class T>
        class IntPtr;
        template<class T>
        class ConstIntPtr;
    }

template<typename T>
struct ConstIntegerBaseTrait<int_vector::IntRef<T>> {
    using Dispatch = typename int_vector::IntRepr<T>::IntOpDispatch;
};
template<typename T>
struct IntegerBaseTrait<int_vector::IntRef<T>> {
    using Dispatch = typename int_vector::IntRepr<T>::IntOpDispatch;
};
template<typename T>
struct ConstIntegerBaseTrait<int_vector::ConstIntRef<T>> {
    using Dispatch = typename int_vector::IntRepr<T>::IntOpDispatch;
};
}

namespace std {
    template<typename T>
    struct iterator_traits<tdc::int_vector::IntPtr<T>> {
        typedef ptrdiff_t                                        difference_type;
        typedef typename tdc::int_vector::IntRepr<T>::value_type value_type;
        typedef tdc::int_vector::IntPtr<T>                       pointer;
        typedef tdc::int_vector::IntRef<T>                       reference;
        typedef std::random_access_iterator_tag                  iterator_category;
    };
    template<typename T>
    struct iterator_traits<tdc::int_vector::ConstIntPtr<T>> {
        typedef ptrdiff_t                                        difference_type;
        typedef typename tdc::int_vector::IntRepr<T>::value_type value_type;
        typedef tdc::int_vector::ConstIntPtr<T>                  pointer;
        typedef tdc::int_vector::ConstIntRef<T>                  reference;
        typedef std::random_access_iterator_tag                  iterator_category;
    };
}

namespace tdc {
namespace int_vector {
    using sdsl::bits;

    template<class T>
    struct IntPtrBase {};

    template<typename T>
    class IntPtrBase<ConstIntPtr<T>>:
        public IntRepr<T>::ConstIntPtrBase {
    public:
        using IntRepr<T>::ConstIntPtrBase::ConstIntPtrBase;
    };

    template<typename T>
    class IntPtrBase<IntPtr<T>>:
        public IntRepr<T>::IntPtrBase {
    public:
        using IntRepr<T>::IntPtrBase::IntPtrBase;

        inline operator IntPtrBase<ConstIntPtr<T>>() const {
            return IntPtrBase<ConstIntPtr<T>>(this->m_ptr, this->m_bit_offset, this->data_bit_size());
        }
    };

    template<class Self, class Ptr>
    class GenericIntRef;

    template<class Self, class T>
    class GenericIntPtr: IntPtrBase<Self> {
    protected:
        friend class GenericIntRef<IntRef<T>, IntPtr<T>>;
        friend class GenericIntRef<ConstIntRef<T>, ConstIntPtr<T>>;
        friend class IntRef<T>;
        friend class ConstIntRef<T>;
        friend struct IntegerBaseTrait<IntRef<T>>;
        friend struct ConstIntegerBaseTrait<IntRef<T>>;
        friend struct ConstIntegerBaseTrait<ConstIntRef<T>>;
        friend struct IntRepr<T>::IntOpDispatch;

    public:
        using tag_type = T;

        GenericIntPtr():
            IntPtrBase<Self>(nullptr, 0, 0) {}
        GenericIntPtr(const IntPtrBase<Self>& other):
            IntPtrBase<Self>(other) {}
        GenericIntPtr(const GenericIntPtr& other):
            IntPtrBase<Self>(other){}

        Self& operator=(const Self& other) {
            this->set_data_bit_size(other.data_bit_size());
            this->m_ptr = other.m_ptr;
            this->m_bit_offset = other.m_bit_offset;
            return static_cast<Self&>(*this);
        }

        Self& operator++() {
            DynamicIntValueType const* tmp = this->m_ptr;
            bits::move_right(tmp, this->m_bit_offset, this->data_bit_size());
            this->m_ptr = (DynamicIntValueType*) tmp;
            return static_cast<Self&>(*this);
        }

        Self& operator--() {
            DynamicIntValueType const* tmp = this->m_ptr;
            bits::move_left(tmp, this->m_bit_offset, this->data_bit_size());
            this->m_ptr = (DynamicIntValueType*) tmp;
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
            DCHECK(lhs.data_bit_size() == rhs.data_bit_size());
            auto ptr_diff = lhs.m_ptr - rhs.m_ptr;
            auto bit_count = ptr_diff * sizeof(DynamicIntValueType) * CHAR_BIT;
            bit_count += lhs.m_bit_offset;
            bit_count -= rhs.m_bit_offset;
            bit_count /= lhs.data_bit_size();
            return bit_count;
        }

        friend bool operator==(const Self& lhs, const Self& rhs) {
            DCHECK(lhs.data_bit_size() == rhs.data_bit_size());
            return (lhs.m_ptr == rhs.m_ptr)
                && (lhs.m_bit_offset == rhs.m_bit_offset);
        }
        friend bool operator<(const Self& lhs, const Self& rhs)  {
            // TODO: test
            DCHECK(lhs.data_bit_size() == rhs.data_bit_size());
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

        ConstIntRef<T> operator[](size_t i) const;
        ConstIntRef<T> operator*() const;
    };

    template<class T>
    class ConstIntPtr: public GenericIntPtr<ConstIntPtr<T>, T> {
    public:
        using GenericIntPtr<ConstIntPtr<T>, T>::GenericIntPtr;
        inline ConstIntPtr(): GenericIntPtr<ConstIntPtr<T>, T>::GenericIntPtr() {}
        inline ConstIntPtr(const ConstIntPtr& other): GenericIntPtr<ConstIntPtr<T>, T>::GenericIntPtr(other) {}

        ConstIntPtr& operator=(const ConstIntPtr& other) {
            return ((GenericIntPtr<ConstIntPtr<T>, T>&) *this) = other;
        }
    };

    template<class T>
    class IntPtr: public GenericIntPtr<IntPtr<T>, T> {
    public:
        using GenericIntPtr<IntPtr<T>, T>::GenericIntPtr;
        inline IntPtr(): GenericIntPtr<IntPtr<T>, T>::GenericIntPtr() {}
        inline IntPtr(const IntPtr& other): GenericIntPtr<IntPtr<T>, T>::GenericIntPtr(other) {}

        IntPtr& operator=(const IntPtr& other) {
            return ((GenericIntPtr<IntPtr<T>, T>&) *this) = other;
        }

        inline IntRef<T> operator*();
        inline IntRef<T> operator[](size_t i);

        inline operator ConstIntPtr<T>() const {
            return ConstIntPtr<T>(IntPtrBase<ConstIntPtr<T>>((const IntPtrBase<IntPtr<T>>&) *this));
        }
    };

    template<class Self, class Ptr>
    class GenericIntRef {
        using T = typename Ptr::tag_type;
    protected:
        friend class IntPtr<T>;
        friend class ConstIntPtr<T>;
        friend struct IntegerBaseTrait<IntRef<T>>;
        friend struct ConstIntegerBaseTrait<IntRef<T>>;
        friend struct ConstIntegerBaseTrait<ConstIntRef<T>>;
        friend struct IntRepr<T>::IntOpDispatch;

        Ptr m_ptr;
    public:
        using value_type = typename IntRepr<T>::value_type;

        inline GenericIntRef() = delete;
        explicit GenericIntRef(const Ptr& ptr): m_ptr(ptr) {}

        operator value_type() const {
            return IntRepr<T>::mem_rw::read_int(
                m_ptr.m_ptr, m_ptr.m_bit_offset, this->m_ptr.data_bit_size());
        }

    };

    template<class T>
    class ConstIntRef:
        public GenericIntRef<ConstIntRef<T>, ConstIntPtr<T>>,
        public ConstIntegerBaseCombiner<
            ConstIntegerBaseWithSelf<ConstIntRef<T>>,
            ConstIntegerBaseWith32<ConstIntRef<T>, unsigned char>,
            ConstIntegerBaseWith32<ConstIntRef<T>, char>,
            ConstIntegerBaseWith32<ConstIntRef<T>, signed char>,
            ConstIntegerBaseWith32<ConstIntRef<T>, unsigned short int>,
            ConstIntegerBaseWith32<ConstIntRef<T>, signed short int>,
            ConstIntegerBaseWith32<ConstIntRef<T>, unsigned int>,
            ConstIntegerBaseWith32<ConstIntRef<T>, signed int>,
            ConstIntegerBaseWith64<ConstIntRef<T>, unsigned long int>,
            ConstIntegerBaseWith64<ConstIntRef<T>, signed long int>,
            ConstIntegerBaseWith64<ConstIntRef<T>, unsigned long long int>,
            ConstIntegerBaseWith64<ConstIntRef<T>, signed long long int>,
            ConstIntegerBaseWith64<ConstIntRef<T>, T>
        > {
    public:
        inline ConstIntRef() = delete;
        explicit ConstIntRef(const ConstIntPtr<T>& ptr): GenericIntRef<ConstIntRef<T>, ConstIntPtr<T>>::GenericIntRef(ptr) {}

        inline ConstIntPtr<T> operator&() { return this->m_ptr; }
    };

    template<class T>
    class IntRef:
        public GenericIntRef<IntRef<T>, IntPtr<T>>,
        public IntegerBaseCombiner<
            IntegerBaseWithSelf<IntRef<T>>,
            IntegerBaseWith32<IntRef<T>, unsigned char>,
            IntegerBaseWith32<IntRef<T>, char>,
            IntegerBaseWith32<IntRef<T>, signed char>,
            IntegerBaseWith32<IntRef<T>, unsigned short int>,
            IntegerBaseWith32<IntRef<T>, signed short int>,
            IntegerBaseWith32<IntRef<T>, unsigned int>,
            IntegerBaseWith32<IntRef<T>, signed int>,
            IntegerBaseWith64<IntRef<T>, unsigned long int>,
            IntegerBaseWith64<IntRef<T>, signed long int>,
            IntegerBaseWith64<IntRef<T>, unsigned long long int>,
            IntegerBaseWith64<IntRef<T>, signed long long int>,
            IntegerBaseWith64<IntRef<T>, T>
        > {
    public:
        using typename GenericIntRef<IntRef<T>, IntPtr<T>>::value_type;

        inline IntRef() = delete;
        explicit IntRef(const IntPtr<T>& ptr): GenericIntRef<IntRef<T>, IntPtr<T>>::GenericIntRef(ptr) {}

        inline IntRef& operator=(value_type other) {
            IntRepr<T>::mem_rw::write_int(
                this->m_ptr.m_ptr, other,
                this->m_ptr.m_bit_offset,
                this->m_ptr.data_bit_size()
            );

            return *this;
        };

        inline IntRef& operator=(const IntRef& other) {
            return operator=(other.operator value_type());
        };

        inline IntRef& operator=(const ConstIntRef<T>& other);

        inline IntPtr<T> operator&() { return this->m_ptr; }

        inline operator ConstIntRef<T>() const {
            return ConstIntRef<T>(this->m_ptr);
        }
    };

    template<class T>
    inline IntRef<T>& IntRef<T>::operator=(const ConstIntRef<T>& other) {
        return operator=(value_type(other));
    }

    template<class T>
    inline IntRef<T> IntPtr<T>::operator*() {
        return IntRef<T>(*this);
    }

    template<class Self, class T>
    ConstIntRef<T> GenericIntPtr<Self, T>::operator*() const {
        return ConstIntRef<T>(static_cast<const Self&>(*this));
    }

    template<class Self, class T>
    ConstIntRef<T> GenericIntPtr<Self, T>::operator[](size_t i) const {
        auto x = *this;
        x += i;
        return ConstIntRef<T>(static_cast<const Self&>(x));
    }

    template<class T>
    inline IntRef<T> IntPtr<T>::operator[](size_t i) {
        auto x = *this;
        x += i;
        return IntRef<T>(x);
    }

    template<class T>
    inline void swap(IntRef<T>& lhs, IntRef<T>& rhs) noexcept {
        T tmp = lhs;
        lhs = rhs.operator T();
        rhs = tmp;
    }

    template<class T>
    inline void swap(IntRef<T>& lhs, T& rhs) noexcept {
        T tmp = lhs;
        lhs = T(rhs);
        rhs = tmp;
    }

    template<class T>
    inline void swap(T& lhs, IntRef<T>& rhs) noexcept {
        T tmp = lhs;
        lhs = rhs.operator T();
        rhs = tmp;
    }

    static_assert(sizeof(GenericIntPtr<ConstIntPtr<uint_t<40>>, uint_t<40>>) <= (sizeof(void*) * 2),
                  "make sure this is reasonably small");

}

template<class T>
using IntRef = int_vector::IntRef<T>;

template<class T>
using ConstIntRef = int_vector::ConstIntRef<T>;

template<class T>
using IntPtr = int_vector::IntPtr<T>;

template<class T>
using ConstIntPtr = int_vector::ConstIntPtr<T>;

}

/// \endcond

