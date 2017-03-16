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

#include <tudocomp/ds/uint_t.hpp>
#include <tudocomp/ds/dynamic_t.hpp>
#include <tudocomp/util/IntegerBase.hpp>

#include <sdsl/bits.hpp>
#include <glog/logging.h>

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

        template<class Self>
        inline void write_int(uint64_t* word, uint64_t x, uint8_t offset, const uint8_t len) {
            sdsl::bits::write_int(word, x, offset, len);
        }
        template<>
        inline void write_int<uint_t<1>>(uint64_t* word, uint64_t v, uint8_t o, const uint8_t len) {
            auto& p = *word;
            const auto mask = uint64_t(1) << o;

            v &= 1;
            p &= (~mask);
            p |= (uint64_t(v) << o);
        }

        template<class Self>
        inline uint64_t read_int(const uint64_t* word, uint8_t offset, const uint8_t len) {
            return sdsl::bits::read_int(word, offset, len);
        }

        template<>
        inline uint64_t read_int<uint_t<1>>(const uint64_t* word, uint8_t o, const uint8_t len) {
            const auto p = *word;
            const auto mask = uint64_t(1) << o;

            return (p & mask) != 0;
        }
    }

template<class MB, class For>
struct RefDispatch {
    typedef MB SelfMaxBit;

    template<class Ref, class V>
    inline static void assign(Ref& self, V v) {
        int_vector::write_int<For>(self.m_ptr.m_ptr,
                                   v,
                                   self.m_ptr.m_bit_offset,
                                   self.m_ptr.data_bit_size());
    };

    template<class Ref, class R>
    inline static R cast_for_op(const Ref& self) {
        return int_vector::read_int<For>(self.m_ptr.m_ptr,
                                         self.m_ptr.m_bit_offset,
                                         self.m_ptr.data_bit_size());
    }
};

template<size_t N>
struct ConstIntegerBaseTrait<int_vector::IntRef<uint_t<N>>, typename std::enable_if<(N <= 32)>::type> {
    typedef RefDispatch<uint32_t, uint_t<N>> Dispatch;
};
template<size_t N>
struct IntegerBaseTrait<int_vector::IntRef<uint_t<N>>, typename std::enable_if<(N <= 32)>::type> {
    typedef RefDispatch<uint32_t, uint_t<N>> Dispatch;
};
template<size_t N>
struct ConstIntegerBaseTrait<int_vector::ConstIntRef<uint_t<N>>, typename std::enable_if<(N <= 32)>::type> {
    typedef RefDispatch<uint32_t, uint_t<N>> Dispatch;
};

template<size_t N>
struct ConstIntegerBaseTrait<int_vector::IntRef<uint_t<N>>, typename std::enable_if<(N > 32)>::type> {
    typedef RefDispatch<uint64_t, uint_t<N>> Dispatch;
};
template<size_t N>
struct IntegerBaseTrait<int_vector::IntRef<uint_t<N>>, typename std::enable_if<(N > 32)>::type> {
    typedef RefDispatch<uint64_t, uint_t<N>> Dispatch;
};
template<size_t N>
struct ConstIntegerBaseTrait<int_vector::ConstIntRef<uint_t<N>>, typename std::enable_if<(N > 32)>::type> {
    typedef RefDispatch<uint64_t, uint_t<N>> Dispatch;
};


template<>
struct ConstIntegerBaseTrait<int_vector::IntRef<dynamic_t>> {
    typedef RefDispatch<uint64_t, dynamic_t> Dispatch;
};
template<>
struct IntegerBaseTrait<int_vector::IntRef<dynamic_t>> {
    typedef RefDispatch<uint64_t, dynamic_t> Dispatch;
};
template<>
struct ConstIntegerBaseTrait<int_vector::ConstIntRef<dynamic_t>> {
    typedef RefDispatch<uint64_t, dynamic_t> Dispatch;
};
}

namespace std {
    template<class T>
    struct iterator_traits<tdc::int_vector::IntPtr<T>> {
        typedef ptrdiff_t                       difference_type;
        typedef T                               value_type;
        typedef tdc::int_vector::IntPtr<T>      pointer;
        typedef tdc::int_vector::IntRef<T>      reference;
        typedef std::random_access_iterator_tag iterator_category;
    };
    template<class T>
    struct iterator_traits<tdc::int_vector::ConstIntPtr<T>> {
        typedef ptrdiff_t                       difference_type;
        typedef T                               value_type;
        typedef tdc::int_vector::ConstIntPtr<T> pointer;
        typedef tdc::int_vector::ConstIntRef<T> reference;
        typedef std::random_access_iterator_tag iterator_category;
    };
}

namespace tdc {
namespace int_vector {
    using sdsl::bits;

    typedef uint64_t DynamicIntValueType;

    template<class T>
    struct IntPtrTrait {};

    template<size_t N>
    struct IntPtrTrait<ConstIntPtr<uint_t<N>>> {
        class Data {
        public:
            const DynamicIntValueType* m_ptr;
            uint8_t m_bit_offset;
        private:
            //const uint8_t m_bit_size;
        public:
            Data(const DynamicIntValueType* ptr, uint8_t offset, uint8_t size):
                m_ptr(ptr), m_bit_offset(offset) /*, m_bit_size(size)*/ {}
            inline uint8_t data_bit_size() const { return N; }
            inline Data data_offset_to(const DynamicIntValueType* ptr, uint8_t offset) const {
                return Data(ptr, offset, this->data_bit_size());
            }
        };
    };

    template<size_t N>
    struct IntPtrTrait<IntPtr<uint_t<N>>> {
        class Data {
        public:
            DynamicIntValueType* m_ptr;
            uint8_t m_bit_offset;
        private:
            //const uint8_t m_bit_size;
        public:
            Data(DynamicIntValueType* ptr, uint8_t offset, uint8_t size):
                m_ptr(ptr), m_bit_offset(offset) /*, m_bit_size(size)*/ {}
            inline uint8_t data_bit_size() const { return N; }
            inline Data data_offset_to(DynamicIntValueType* ptr, uint8_t offset) {
                return Data(ptr, offset, this->data_bit_size());
            }
            inline operator typename IntPtrTrait<ConstIntPtr<uint_t<N>>>::Data() const {
                return typename IntPtrTrait<ConstIntPtr<uint_t<N>>>::Data(m_ptr, m_bit_offset, 0);
            }
        };
    };

    template<>
    struct IntPtrTrait<ConstIntPtr<dynamic_t>> {
        class Data {
        public:
            const DynamicIntValueType* m_ptr;
            uint8_t m_bit_offset;
        private:
            const uint8_t m_bit_size;
        public:
            Data(const DynamicIntValueType* ptr, uint8_t offset, uint8_t size):
                m_ptr(ptr), m_bit_offset(offset), m_bit_size(size) {}
            inline uint8_t data_bit_size() const { return m_bit_size; }
            inline Data data_offset_to(const DynamicIntValueType* ptr, uint8_t offset) const {
                return Data(ptr, offset, this->data_bit_size());
            }
        };
    };

    template<>
    struct IntPtrTrait<IntPtr<dynamic_t>> {
        class Data {
        public:
            DynamicIntValueType* m_ptr;
            uint8_t m_bit_offset;
        private:
            const uint8_t m_bit_size;
        public:
            Data(DynamicIntValueType* ptr, uint8_t offset, uint8_t size):
                m_ptr(ptr), m_bit_offset(offset), m_bit_size(size) {}
            inline uint8_t data_bit_size() const { return m_bit_size; }
            inline Data data_offset_to(DynamicIntValueType* ptr, uint8_t offset) const {
                return Data(ptr, offset, this->data_bit_size());
            }
            inline operator typename IntPtrTrait<ConstIntPtr<dynamic_t>>::Data() const {
                return typename IntPtrTrait<ConstIntPtr<dynamic_t>>::Data(m_ptr, m_bit_offset, m_bit_size);
            }
        };
    };

    template<class Self, class Ptr, class T>
    class GenericIntRef;

    template<class Self, class T>
    class GenericIntPtr: IntPtrTrait<Self>::Data {
    protected:
        friend class GenericIntRef<IntRef<T>, IntPtr<T>, T>;
        friend class GenericIntRef<ConstIntRef<T>, ConstIntPtr<T>, T>;
        friend class IntRef<T>;
        friend class ConstIntRef<T>;
        friend struct IntegerBaseTrait<IntRef<T>>;
        friend struct ConstIntegerBaseTrait<IntRef<T>>;
        friend struct ConstIntegerBaseTrait<ConstIntRef<T>>;
        friend struct RefDispatch<uint32_t, T>;
        friend struct RefDispatch<uint64_t, T>;

    public:
        GenericIntPtr():
            IntPtrTrait<Self>::Data(nullptr, 0, 0) {}
        GenericIntPtr(const typename IntPtrTrait<Self>::Data& other):
            IntPtrTrait<Self>::Data(other) {}
        GenericIntPtr(const GenericIntPtr& other):
            IntPtrTrait<Self>::Data(other){}

        Self& operator=(const Self& other) {
            DCHECK(this->data_bit_size() == other.data_bit_size());
            this->m_ptr = other.m_ptr;
            this->m_bit_offset = other.m_bit_offset;
            return static_cast<Self&>(*this);
        }

        Self& operator++() {
            const DynamicIntValueType* tmp = this->m_ptr;
            bits::move_right(tmp, this->m_bit_offset, this->data_bit_size());
            this->m_ptr = (DynamicIntValueType*) tmp;
            return static_cast<Self&>(*this);
        }

        Self& operator--() {
            const DynamicIntValueType* tmp = this->m_ptr;
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
            return ConstIntPtr<T>(typename IntPtrTrait<ConstIntPtr<T>>::Data((const typename IntPtrTrait<IntPtr<T>>::Data&) *this));
        }
    };

    template<typename T>
    class GenericIntRefAutocast {
    public:
        using autocast_type = T;
    };

    template<class Self, class Ptr, class T>
    class GenericIntRef {
    protected:
        friend class IntPtr<T>;
        friend class ConstIntPtr<T>;
        friend struct IntegerBaseTrait<IntRef<T>>;
        friend struct ConstIntegerBaseTrait<IntRef<T>>;
        friend struct ConstIntegerBaseTrait<ConstIntRef<T>>;
        friend struct RefDispatch<uint32_t, T>;
        friend struct RefDispatch<uint64_t, T>;

        Ptr m_ptr;
    public:
        using value_type = T;
        using autocast_type = typename GenericIntRefAutocast<T>::autocast_type;

        inline GenericIntRef() = delete;
        explicit GenericIntRef(const Ptr& ptr): m_ptr(ptr) {}

        operator autocast_type() const {
            return read_int<autocast_type>(
                m_ptr.m_ptr, m_ptr.m_bit_offset, this->m_ptr.data_bit_size());
        }

    };

    template<class T>
    class ConstIntRef:
        public GenericIntRef<ConstIntRef<T>, ConstIntPtr<T>, T>,
        public ConstIntegerBaseCombiner<
            ConstIntegerBaseWithSelf<ConstIntRef<T>>,
            ConstIntegerBaseWith32<ConstIntRef<T>, uint32_t>,
            ConstIntegerBaseWith32<ConstIntRef<T>, int>,
            ConstIntegerBaseWith64<ConstIntRef<T>, uint64_t>,
            ConstIntegerBaseWith64<ConstIntRef<T>, T>
        > {
    public:
        inline ConstIntRef() = delete;
        explicit ConstIntRef(const ConstIntPtr<T>& ptr): GenericIntRef<ConstIntRef<T>, ConstIntPtr<T>, T>::GenericIntRef(ptr) {}

        inline ConstIntPtr<T> operator&() { return this->m_ptr; }
    };

    template<class T>
    class IntRef:
        public GenericIntRef<IntRef<T>, IntPtr<T>, T>,
        public IntegerBaseCombiner<
            IntegerBaseWithSelf<IntRef<T>>,
            IntegerBaseWith32<IntRef<T>, uint32_t>,
            IntegerBaseWith32<IntRef<T>, int>,
            IntegerBaseWith64<IntRef<T>, uint64_t>,
            IntegerBaseWith64<IntRef<T>, T>
        > {
    public:
        using typename GenericIntRef<IntRef<T>, IntPtr<T>, T>::value_type;
        using autocast_type = typename GenericIntRefAutocast<T>::autocast_type;

        inline IntRef() = delete;
        explicit IntRef(const IntPtr<T>& ptr): GenericIntRef<IntRef<T>, IntPtr<T>, T>::GenericIntRef(ptr) {}

        inline IntRef& operator=(value_type other) {
            write_int<T>(this->m_ptr.m_ptr, other,
                         this->m_ptr.m_bit_offset,
                         this->m_ptr.data_bit_size());
            return *this;
        };

        inline IntRef& operator=(const IntRef& other) {
            return operator=(value_type(other.operator autocast_type()));
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
    };

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

    // specializations for dynamic_t
    template<> class GenericIntRefAutocast<dynamic_t> {
    public:
        using autocast_type = uint64_t;
    };

    // specializations for uint_t<1>
    template<> class GenericIntRefAutocast<uint_t<1>> {
    public:
        using autocast_type = bool;
    };
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
