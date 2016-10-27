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
        class IntRef;
        class ConstIntRef;
        class IntPtr;
        class ConstIntPtr;
    }
}

namespace std {
    template<>
    struct iterator_traits<tdc::int_vector::IntPtr> {
        typedef ptrdiff_t                       difference_type;
        // TODO: Fix if pointers become generic
        typedef uint64_t                        value_type;
        typedef tdc::int_vector::IntPtr         pointer;
        typedef tdc::int_vector::IntRef         reference;
        typedef std::random_access_iterator_tag iterator_category;
    };
    template<>
    struct iterator_traits<tdc::int_vector::ConstIntPtr> {
        typedef ptrdiff_t                       difference_type;
        // TODO: Fix if pointers become generic
        typedef uint64_t                        value_type;
        typedef tdc::int_vector::ConstIntPtr    pointer;
        typedef tdc::int_vector::ConstIntRef    reference;
        typedef std::random_access_iterator_tag iterator_category;
    };
}

namespace tdc {

template<>
struct IntegerBaseTraitConst<int_vector::IntRef> {
    typedef sdsl::bits bits;
    typedef int_vector::IntRef T;

    typedef uint64_t SelfMaxBit;

    inline static SelfMaxBit cast_for_self_op(const T& self);
    inline static SelfMaxBit cast_for_32_op(const T& self);
    inline static uint64_t cast_for_64_op(const T& self);
};

template<>
struct IntegerBaseTrait<int_vector::IntRef> {
    typedef sdsl::bits bits;
    typedef int_vector::IntRef T;

    inline static void assign(T& self, uint32_t v);
    inline static void assign(T& self, uint64_t v);
};

template<>
struct IntegerBaseTraitConst<int_vector::ConstIntRef> {
    typedef sdsl::bits bits;
    typedef int_vector::ConstIntRef T;

    typedef uint64_t SelfMaxBit;

    inline static SelfMaxBit cast_for_self_op(const T& self);
    inline static SelfMaxBit cast_for_32_op(const T& self);
    inline static uint64_t cast_for_64_op(const T& self);
};

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

    template<class Self, class Ptr, class Layout>
    class GenericIntRef;
    class IntRef;
    class ConstIntRef;
    class IntPtr;
    class ConstIntPtr;

    template<class Self, class Layout>
    class GenericIntPtr {
    protected:
        friend class GenericIntRef<IntRef, IntPtr, Mut>;
        friend class GenericIntRef<ConstIntRef, ConstIntPtr, Const>;
        friend class IntRef;
        friend class ConstIntRef;
        friend class IntegerBaseTrait<IntRef>;
        friend class IntegerBaseTraitConst<IntRef>;
        friend class IntegerBaseTraitConst<ConstIntRef>;

        typename Layout::ptr m_ptr;
        typename Layout::off m_bit_offset;
        typename Layout::len m_bit_size;
    public:
        GenericIntPtr(typename Layout::ptr ptr,
                      typename Layout::off bit_offset,
                      typename Layout::len bit_size):
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
            auto bit_count = ptr_diff * sizeof(typename Layout::val) * CHAR_BIT;
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

    template<class Self, class Ptr, class Layout>
    class GenericIntRef {
    protected:
        friend class IntPtr;
        friend class ConstIntPtr;
        friend class IntegerBaseTrait<IntRef>;
        friend class IntegerBaseTraitConst<IntRef>;
        friend class IntegerBaseTraitConst<ConstIntRef>;

        Ptr m_ptr;
    public:
        typedef typename Layout::val value_type;

        explicit GenericIntRef(const Ptr& ptr): m_ptr(ptr) {}

        operator value_type() const {
            return bits::read_int(m_ptr.m_ptr, m_ptr.m_bit_offset, m_ptr.m_bit_size);
        }

    };

    class IntRef: public GenericIntRef<IntRef, IntPtr, Mut>, public IntegerBase<IntRef> {
    public:
        explicit IntRef(const IntPtr& ptr): GenericIntRef(ptr) {}

        IntRef& operator=(value_type other)
        {
            bits::write_int(m_ptr.m_ptr, other, m_ptr.m_bit_offset, m_ptr.m_bit_size);
            return *this;
        };

    };

    class ConstIntRef: public GenericIntRef<ConstIntRef, ConstIntPtr, Const>, public ConstIntegerBase<ConstIntRef> {
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

    // TODO: uint64_t should be a type corresponding to the element type,
    // eg uint_t<5>
    inline void swap(IntRef& lhs, IntRef& rhs) noexcept {
        uint64_t tmp = lhs;
        lhs = uint64_t(rhs);
        rhs = tmp;
    }

    // TODO: uint64_t should be a type corresponding to the element type,
    // eg uint_t<5>
    inline void swap(IntRef& lhs, uint64_t& rhs) noexcept {
        uint64_t tmp = lhs;
        lhs = uint64_t(rhs);
        rhs = tmp;
    }

    // TODO: uint64_t should be a type corresponding to the element type,
    // eg uint_t<5>
    inline void swap(uint64_t& lhs, IntRef& rhs) noexcept {
        uint64_t tmp = lhs;
        lhs = uint64_t(rhs);
        rhs = tmp;
    }

    static_assert(sizeof(GenericIntPtr<ConstIntPtr, Const>) <= (sizeof(void*) * 2), "make sure this is reasonably small");

    template<class T>
    struct even_bit_backing_data {
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

        std::vector<T> m_vec;

        inline explicit even_bit_backing_data() {}
        inline explicit even_bit_backing_data(size_type n): m_vec(n) {}
        inline even_bit_backing_data(size_type n, const value_type& val): m_vec(n, val) {}
        template <class InputIterator>
        inline even_bit_backing_data (InputIterator first, InputIterator last): m_vec(first, last) {}
        inline even_bit_backing_data (const even_bit_backing_data& other): m_vec(other.m_vec) {}
        inline even_bit_backing_data (even_bit_backing_data&& other): m_vec(std::move(other.m_vec)) {}
        inline even_bit_backing_data (std::initializer_list<value_type> il): m_vec(il) {}

        inline even_bit_backing_data& operator=(const even_bit_backing_data& other) {
            m_vec = other.m_vec;
            return *this;
        }

        inline even_bit_backing_data& operator=(even_bit_backing_data&& other) {
            m_vec = std::move(other.m_vec);
            return *this;
        }

        inline even_bit_backing_data& operator=(std::initializer_list<value_type> il) {
            m_vec = il;
            return *this;
        }

        inline iterator begin() {
            return m_vec.begin();
        }

        inline iterator end() {
            return m_vec.end();
        }

        inline reverse_iterator rbegin() {
            return m_vec.rbegin();
        }

        inline reverse_iterator rend() {
            return m_vec.rend();
        }

        inline const_iterator begin() const {
            return m_vec.begin();
        }

        inline const_iterator end() const {
            return m_vec.end();
        }

        inline const_reverse_iterator rbegin() const {
            return m_vec.rbegin();
        }

        inline const_reverse_iterator rend() const {
            return m_vec.rend();
        }

        inline const_iterator cbegin() const {
            return m_vec.cbegin();
        }

        inline const_iterator cend() const {
            return m_vec.cend();
        }

        inline const_reverse_iterator crbegin() const {
            return m_vec.crbegin();
        }

        inline const_reverse_iterator crend() const {
            return m_vec.crend();
        }
    };

    template<size_t N>
    struct odd_bit_backing_data {
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

        std::vector<DynamicIntValueType> m_vec;
        uint64_t m_bit_size;

        inline static uint64_t backing2bits(size_t n) {
            return uint64_t(sizeof(DynamicIntValueType) * CHAR_BIT) * uint64_t(n);
        }

        inline static uint64_t elem2bits(size_t n) {
            return uint64_t(N) * uint64_t(n);
        }

        inline static uint64_t bits2backing(uint64_t bits) {
            if (bits == 0) {
                return 0;
            }
            return ((bits - 1) / backing2bits(1)) + 1;
        }

        struct PosAndOffset { size_t elem_pos; uint8_t elem_offset; };
        inline PosAndOffset bitpos2elempos(uint64_t bits) {
            return PosAndOffset {
                bits / backing2bits(1),
                bits % backing2bits(1)
            };
        }

        inline explicit odd_bit_backing_data() {}
        inline explicit odd_bit_backing_data(size_type n) {
            m_bit_size = elem2bits(n);
            size_t converted_size = bits2backing(m_bit_size);
            m_vec = std::vector<DynamicIntValueType>(converted_size);
        }
        inline odd_bit_backing_data(size_type n, const value_type& val): odd_bit_backing_data(n) {
            auto ptr = m_vec.data();
            uint8_t offset = 0;

            for (size_t i = 0; i < n; i++) {
                bits::write_int_and_move(ptr, val, offset, N);
            }
        }
        template <class InputIterator>
        inline odd_bit_backing_data(InputIterator first, InputIterator last) {
            // TODO: specialize for random acces iterator
            for(; first != last; first++) {
                //TODO: push_back(*first);
                // TODO: Update bit_size
            }
        }
        inline odd_bit_backing_data (const odd_bit_backing_data& other):
            m_vec(other.m_vec), m_bit_size(other.m_bit_size) {}
        inline odd_bit_backing_data (odd_bit_backing_data&& other):
            m_vec(std::move(other.m_vec)), m_bit_size(other.m_bit_size) {}
        inline odd_bit_backing_data(std::initializer_list<value_type> il):
            odd_bit_backing_data(il.begin(), il.end()) {}

        inline odd_bit_backing_data& operator=(const odd_bit_backing_data& other) {
            m_vec = other.m_vec;
            m_bit_size = other.m_bit_size;
            return *this;
        }

        inline odd_bit_backing_data& operator=(odd_bit_backing_data&& other) {
            m_vec = std::move(other.m_vec);
            m_bit_size = other.m_bit_size;
            return *this;
        }

        inline odd_bit_backing_data& operator=(std::initializer_list<value_type> il) {
            *this = odd_bit_backing_data(il);
            return *this;
        }

        inline iterator begin() {
            auto x = bitpos2elempos(0);
            return pointer(&m_vec[x.elem_pos], x.elem_offset, N);
        }

        inline iterator end() {
            auto x = bitpos2elempos(m_bit_size);
            return pointer(&m_vec[x.elem_pos], x.elem_offset, N);
        }

        inline reverse_iterator rbegin() {
            return std::reverse_iterator<iterator>(end());
        }

        inline reverse_iterator rend() {
            return std::reverse_iterator<iterator>(begin());
        }

        inline const_iterator begin() const {
            auto x = bitpos2elempos(0);
            return const_pointer(&m_vec[x.elem_pos], x.elem_offset, N);
        }

        inline const_iterator end() const {
            auto x = bitpos2elempos(m_bit_size);
            return const_pointer(&m_vec[x.elem_pos], x.elem_offset, N);
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
    };

    template<class T, class X = void>
    struct GenericIntVectorTrait {
        typedef typename even_bit_backing_data<T>::value_type             value_type;

        typedef typename even_bit_backing_data<T>::reference              reference;
        typedef typename even_bit_backing_data<T>::const_reference        const_reference;

        typedef typename even_bit_backing_data<T>::pointer                pointer;
        typedef typename even_bit_backing_data<T>::const_pointer          const_pointer;

        typedef typename even_bit_backing_data<T>::iterator               iterator;
        typedef typename even_bit_backing_data<T>::const_iterator         const_iterator;

        typedef typename even_bit_backing_data<T>::reverse_iterator       reverse_iterator;
        typedef typename even_bit_backing_data<T>::const_reverse_iterator const_reverse_iterator;

        typedef typename even_bit_backing_data<T>::difference_type        difference_type;
        typedef typename even_bit_backing_data<T>::size_type              size_type;

        typedef          even_bit_backing_data<T>                         backing_data;
    };

    template<size_t N>
    struct GenericIntVectorTrait<uint_t<N>, typename std::enable_if<(N % 8) == 0>::type> {
        typedef typename even_bit_backing_data<uint_t<N>>::value_type             value_type;

        typedef typename even_bit_backing_data<uint_t<N>>::reference              reference;
        typedef typename even_bit_backing_data<uint_t<N>>::const_reference        const_reference;

        typedef typename even_bit_backing_data<uint_t<N>>::pointer                pointer;
        typedef typename even_bit_backing_data<uint_t<N>>::const_pointer          const_pointer;

        typedef typename even_bit_backing_data<uint_t<N>>::iterator               iterator;
        typedef typename even_bit_backing_data<uint_t<N>>::const_iterator         const_iterator;

        typedef typename even_bit_backing_data<uint_t<N>>::reverse_iterator       reverse_iterator;
        typedef typename even_bit_backing_data<uint_t<N>>::const_reverse_iterator const_reverse_iterator;

        typedef typename even_bit_backing_data<uint_t<N>>::difference_type        difference_type;
        typedef typename even_bit_backing_data<uint_t<N>>::size_type              size_type;

        typedef          even_bit_backing_data<uint_t<N>>                         backing_data;
    };

    template<size_t N>
    struct GenericIntVectorTrait<uint_t<N>, typename std::enable_if<(N % 8) != 0>::type> {
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

        typedef odd_bit_backing_data<N>                              backing_data;
    };

    template<class T>
    class GenericIntVector {
        // TODO: Add custom allocator support

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

        typename GenericIntVectorTrait<T>::backing_data m_data;
    public:
        // default
        inline explicit GenericIntVector() {}

        // fill
        explicit GenericIntVector(size_type n): m_data(n) {}
        inline GenericIntVector(size_type n, const value_type& val): m_data(n, val) {}

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
    };

}

inline IntegerBaseTraitConst<int_vector::IntRef>::SelfMaxBit IntegerBaseTraitConst<int_vector::IntRef>::cast_for_self_op(const T& self) {
    return bits::read_int(self.m_ptr.m_ptr,
                            self.m_ptr.m_bit_offset,
                            self.m_ptr.m_bit_size);
}
inline IntegerBaseTraitConst<int_vector::IntRef>::SelfMaxBit IntegerBaseTraitConst<int_vector::IntRef>::cast_for_32_op(const T& self) {
    return bits::read_int(self.m_ptr.m_ptr,
                            self.m_ptr.m_bit_offset,
                            self.m_ptr.m_bit_size);
}
inline uint64_t IntegerBaseTraitConst<int_vector::IntRef>::cast_for_64_op(const T& self) {
    return bits::read_int(self.m_ptr.m_ptr,
                            self.m_ptr.m_bit_offset,
                            self.m_ptr.m_bit_size);
}
inline void IntegerBaseTrait<int_vector::IntRef>::assign(T& self, uint32_t v) {
    bits::write_int(self.m_ptr.m_ptr,
                    v,
                    self.m_ptr.m_bit_offset,
                    self.m_ptr.m_bit_size);
}
inline void IntegerBaseTrait<int_vector::IntRef>::assign(T& self, uint64_t v) {
    bits::write_int(self.m_ptr.m_ptr,
                    v,
                    self.m_ptr.m_bit_offset,
                    self.m_ptr.m_bit_size);
}

inline IntegerBaseTraitConst<int_vector::ConstIntRef>::SelfMaxBit IntegerBaseTraitConst<int_vector::ConstIntRef>::cast_for_self_op(const T& self) {
    return bits::read_int(self.m_ptr.m_ptr,
                            self.m_ptr.m_bit_offset,
                            self.m_ptr.m_bit_size);
}
inline IntegerBaseTraitConst<int_vector::ConstIntRef>::SelfMaxBit IntegerBaseTraitConst<int_vector::ConstIntRef>::cast_for_32_op(const T& self) {
    return bits::read_int(self.m_ptr.m_ptr,
                            self.m_ptr.m_bit_offset,
                            self.m_ptr.m_bit_size);
}
inline uint64_t IntegerBaseTraitConst<int_vector::ConstIntRef>::cast_for_64_op(const T& self) {
    return bits::read_int(self.m_ptr.m_ptr,
                            self.m_ptr.m_bit_offset,
                            self.m_ptr.m_bit_size);
}

}

#endif
