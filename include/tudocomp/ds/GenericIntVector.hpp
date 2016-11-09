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
    }

template<class MB>
struct RefDispatch {
    typedef MB SelfMaxBit;

    template<class Ref, class V>
    inline static void assign(Ref& self, V v) {
        sdsl::bits::write_int(self.m_ptr.m_ptr,
                              v,
                              self.m_ptr.m_bit_offset,
                              self.m_ptr.data_bit_size());
    };

    template<class Ref, class R>
    inline static R cast_for_op(const Ref& self) {
        return sdsl::bits::read_int(self.m_ptr.m_ptr,
                                    self.m_ptr.m_bit_offset,
                                    self.m_ptr.data_bit_size());
    }
};

template<size_t N>
struct ConstIntegerBaseTrait<int_vector::IntRef<uint_t<N>>, typename std::enable_if<(N <= 32)>::type> {
    typedef RefDispatch<uint32_t> Dispatch;
};

template<size_t N>
struct IntegerBaseTrait<int_vector::IntRef<uint_t<N>>, typename std::enable_if<(N <= 32)>::type> {
    typedef RefDispatch<uint32_t> Dispatch;
};

template<size_t N>
struct ConstIntegerBaseTrait<int_vector::ConstIntRef<uint_t<N>>, typename std::enable_if<(N <= 32)>::type> {
    typedef RefDispatch<uint32_t> Dispatch;
};

template<size_t N>
struct ConstIntegerBaseTrait<int_vector::IntRef<uint_t<N>>, typename std::enable_if<(N > 32)>::type> {
    typedef RefDispatch<uint64_t> Dispatch;
};

template<size_t N>
struct IntegerBaseTrait<int_vector::IntRef<uint_t<N>>, typename std::enable_if<(N > 32)>::type> {
    typedef RefDispatch<uint64_t> Dispatch;
};

template<size_t N>
struct ConstIntegerBaseTrait<int_vector::ConstIntRef<uint_t<N>>, typename std::enable_if<(N > 32)>::type> {
    typedef RefDispatch<uint64_t> Dispatch;
};

template<>
struct ConstIntegerBaseTrait<int_vector::IntRef<dynamic_t>> {
    typedef RefDispatch<uint64_t> Dispatch;
};

template<>
struct IntegerBaseTrait<int_vector::IntRef<dynamic_t>> {
    typedef RefDispatch<uint64_t> Dispatch;
};

template<>
struct ConstIntegerBaseTrait<int_vector::ConstIntRef<dynamic_t>> {
    typedef RefDispatch<uint64_t> Dispatch;
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
    struct IntPtrTrait {
    };

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
            inline Data data_offset_to(const DynamicIntValueType* ptr, uint8_t offset) {
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
            inline Data data_offset_to(const DynamicIntValueType* ptr, uint8_t offset) {
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
            inline Data data_offset_to(DynamicIntValueType* ptr, uint8_t offset) {
                return Data(ptr, offset, this->data_bit_size());
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
        friend class IntegerBaseTrait<IntRef<T>>;
        friend class ConstIntegerBaseTrait<IntRef<T>>;
        friend class ConstIntegerBaseTrait<ConstIntRef<T>>;
        friend class RefDispatch<uint32_t>;
        friend class RefDispatch<uint64_t>;

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
    };

    template<class Self, class Ptr, class T>
    class GenericIntRef {
    protected:
        friend class IntPtr<T>;
        friend class ConstIntPtr<T>;
        friend class IntegerBaseTrait<IntRef<T>>;
        friend class ConstIntegerBaseTrait<IntRef<T>>;
        friend class ConstIntegerBaseTrait<ConstIntRef<T>>;
        friend class RefDispatch<uint32_t>;
        friend class RefDispatch<uint64_t>;

        Ptr m_ptr;
    public:
        typedef T value_type;

        inline GenericIntRef() = delete;
        explicit GenericIntRef(const Ptr& ptr): m_ptr(ptr) {}

        operator value_type() const {
            return bits::read_int(m_ptr.m_ptr, m_ptr.m_bit_offset, this->m_ptr.data_bit_size());
        }

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

        inline IntRef() = delete;
        explicit IntRef(const IntPtr<T>& ptr): GenericIntRef<IntRef<T>, IntPtr<T>, T>::GenericIntRef(ptr) {}

        inline IntRef& operator=(value_type other) {
            bits::write_int(this->m_ptr.m_ptr, other,
                            this->m_ptr.m_bit_offset,
                            this->m_ptr.data_bit_size());
            return *this;
        };

        inline IntRef& operator=(const IntRef& other) {
            return operator=(value_type(other.operator value_type()));
        };

        inline IntRef& operator=(const ConstIntRef<T>& other);

        inline IntPtr<T> operator&() { return this->m_ptr; }
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

    enum class ElementStorageMode {
        Direct,
        BitPacked
    };

    inline void width_error() {
        throw std::runtime_error("Can not set the width of a IntVector with statically sized elements");
    }

    template<class T>
    struct OddBitBackingBase {};

    template<size_t N>
    struct OddBitBackingBase<uint_t<N>> {
        typedef DynamicIntValueType internal_data_type;
        typedef uint_t<N>           value_type;

        std::vector<internal_data_type> m_vec;
        uint64_t m_real_size;

        inline OddBitBackingBase():
            m_vec(), m_real_size(0) {}
        inline OddBitBackingBase(const OddBitBackingBase& other):
            m_vec(other.m_vec), m_real_size(other.m_real_size) {}
        inline OddBitBackingBase(OddBitBackingBase&& other):
            m_vec(std::move(other.m_vec)), m_real_size(other.m_real_size) {}

        inline uint8_t width() const { return N; }
        inline void set_width_raw(uint8_t width) { }

    };

    template<>
    struct OddBitBackingBase<dynamic_t> {
        typedef DynamicIntValueType internal_data_type;
        typedef dynamic_t           value_type;

        std::vector<internal_data_type> m_vec;
        uint64_t m_real_size;
        uint8_t m_width;

        inline OddBitBackingBase():
            m_vec(), m_real_size(0), m_width(64) {}
        inline OddBitBackingBase(const OddBitBackingBase& other):
            m_vec(other.m_vec), m_real_size(other.m_real_size), m_width(other.m_width) {}
        inline OddBitBackingBase(OddBitBackingBase&& other):
            m_vec(std::move(other.m_vec)), m_real_size(other.m_real_size), m_width(other.m_width) {}

        inline uint8_t width() const { return m_width; }
        inline void set_width_raw(uint8_t width) { m_width = width; }

    };

    template<class T>
    struct odd_bit_backing_data: OddBitBackingBase<T> {
        typedef typename OddBitBackingBase<T>::value_type            value_type;

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

        typedef typename OddBitBackingBase<T>::internal_data_type internal_data_type;

        template<class M>
        friend bool operator==(const odd_bit_backing_data<M>& lhs, const odd_bit_backing_data<M>& rhs);

        inline uint64_t backing2bits(size_t n) const {
            return uint64_t(sizeof(internal_data_type) * CHAR_BIT) * uint64_t(n);
        }

        inline uint64_t elem2bits(size_t n) const {
            return uint64_t(this->width()) * uint64_t(n);
        }

        inline uint64_t bits2backing(uint64_t bits) const {
            if (bits == 0) {
                return 0;
            }
            return ((bits - 1) / backing2bits(1)) + 1;
        }

        inline uint64_t bits2elem(uint64_t bits) const {
            if (bits == 0) {
                return 0;
            }
            return ((bits - 1) / elem2bits(1)) + 1;
        }

        struct PosAndOffset { size_t pos; uint8_t offset; };
        inline PosAndOffset bitpos2backingpos(uint64_t bits) const {
            return PosAndOffset {
                bits / backing2bits(1),
                uint8_t(bits % backing2bits(1))
            };
        }

        inline explicit odd_bit_backing_data(): OddBitBackingBase<T>::OddBitBackingBase() {}
        inline explicit odd_bit_backing_data(size_type n): odd_bit_backing_data() {
            this->m_real_size = n;
            size_t converted_size = bits2backing(elem2bits(this->m_real_size));
            this->m_vec = std::vector<internal_data_type>(converted_size);
        }
        inline odd_bit_backing_data(size_type n, const value_type& val): odd_bit_backing_data(n) {
            auto ptr = this->m_vec.data();
            uint8_t offset = 0;

            for (size_t i = 0; i < n; i++) {
                bits::write_int_and_move(ptr, val, offset, this->width());
            }
        }

        inline odd_bit_backing_data(size_type n, const value_type& val, uint8_t width):
            odd_bit_backing_data()
        {
            this->set_width_raw(width);
            this->resize(n, val);
        }

        template <class InputIterator>
        inline odd_bit_backing_data(InputIterator first, InputIterator last) {
            // TODO: specialize for random access iterator
            for(; first != last; first++) {
                push_back(*first);
            }
        }
        inline odd_bit_backing_data (const odd_bit_backing_data& other):
            OddBitBackingBase<T>(other) {}
        inline odd_bit_backing_data (odd_bit_backing_data&& other):
            OddBitBackingBase<T>(std::move(other)) {}
        inline odd_bit_backing_data(std::initializer_list<value_type> il):
            odd_bit_backing_data(il.begin(), il.end()) {}

        inline odd_bit_backing_data& operator=(const odd_bit_backing_data& other) {
            this->m_vec = other.m_vec;
            this->m_real_size = other.m_real_size;
            this->set_width_raw(other.width());
            return *this;
        }

        inline odd_bit_backing_data& operator=(odd_bit_backing_data&& other) {
            this->m_vec = std::move(other.m_vec);
            this->m_real_size = other.m_real_size;
            this->set_width_raw(other.width());
            return *this;
        }

        inline odd_bit_backing_data& operator=(std::initializer_list<value_type> il) {
            *this = odd_bit_backing_data(il);
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
            DCHECK(n < size());
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
                ss << "Out-of-range access of GenericIntVector: index is ";
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
            *this = odd_bit_backing_data(first, last);
        }

        inline void assign(size_type n, const value_type& val) {
            *this = odd_bit_backing_data(n, val);
        }

        inline void assign(std::initializer_list<value_type> il) {
            *this = odd_bit_backing_data(il);
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

        inline void swap(odd_bit_backing_data& other) {
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
    };

    template<class N>
    bool operator==(const odd_bit_backing_data<N>& lhs, const odd_bit_backing_data<N>& rhs) {
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
    bool operator!=(const odd_bit_backing_data<N>& lhs, const odd_bit_backing_data<N>& rhs) {
        return !(lhs == rhs);
    }

    template<class N>
    bool operator<(const odd_bit_backing_data<N>& lhs, const odd_bit_backing_data<N>& rhs) {
        return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
    }

    template<class N>
    bool operator<=(const odd_bit_backing_data<N>& lhs, const odd_bit_backing_data<N>& rhs) {
        return !(lhs > rhs);
    }

    template<class N>
    bool operator>(const odd_bit_backing_data<N>& lhs, const odd_bit_backing_data<N>& rhs) {
        return std::lexicographical_compare(rhs.cbegin(), rhs.cend(), lhs.cbegin(), lhs.cend());
    }

    template<class N>
    bool operator>=(const odd_bit_backing_data<N>& lhs, const odd_bit_backing_data<N>& rhs) {
        return !(lhs < rhs);
    }

    template<class N>
    void swap(odd_bit_backing_data<N>& x, odd_bit_backing_data<N>& y) {
        x.swap(y);
    }

    /*
     * TODO:
     o constructor for int width
     o swap/reassign ops
     - void bit_resize(const size_type size);
     - bit capacity?
     - value_type get_int(size_type idx, const uint8_t len=64) const;
     - void set_int(size_type idx, value_type x, const uint8_t len=64);
     - width setter
     - in.place widt setting?
     - flip?
     . bit resize
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

    };

    template<>
    struct GenericIntVectorTrait<dynamic_t> {
        typedef typename odd_bit_backing_data<dynamic_t>::value_type             value_type;

        typedef typename odd_bit_backing_data<dynamic_t>::reference              reference;
        typedef typename odd_bit_backing_data<dynamic_t>::const_reference        const_reference;

        typedef typename odd_bit_backing_data<dynamic_t>::pointer                pointer;
        typedef typename odd_bit_backing_data<dynamic_t>::const_pointer          const_pointer;

        typedef typename odd_bit_backing_data<dynamic_t>::iterator               iterator;
        typedef typename odd_bit_backing_data<dynamic_t>::const_iterator         const_iterator;

        typedef typename odd_bit_backing_data<dynamic_t>::reverse_iterator       reverse_iterator;
        typedef typename odd_bit_backing_data<dynamic_t>::const_reverse_iterator const_reverse_iterator;

        typedef typename odd_bit_backing_data<dynamic_t>::difference_type        difference_type;
        typedef typename odd_bit_backing_data<dynamic_t>::size_type              size_type;

        typedef          odd_bit_backing_data<dynamic_t>                         backing_data;
        typedef typename odd_bit_backing_data<dynamic_t>::internal_data_type     internal_data_type;

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

    };

    template<size_t N>
    struct GenericIntVectorTrait<uint_t<N>, typename std::enable_if<(N % 8) != 0>::type> {
        typedef typename odd_bit_backing_data<uint_t<N>>::value_type             value_type;

        typedef typename odd_bit_backing_data<uint_t<N>>::reference              reference;
        typedef typename odd_bit_backing_data<uint_t<N>>::const_reference        const_reference;

        typedef typename odd_bit_backing_data<uint_t<N>>::pointer                pointer;
        typedef typename odd_bit_backing_data<uint_t<N>>::const_pointer          const_pointer;

        typedef typename odd_bit_backing_data<uint_t<N>>::iterator               iterator;
        typedef typename odd_bit_backing_data<uint_t<N>>::const_iterator         const_iterator;

        typedef typename odd_bit_backing_data<uint_t<N>>::reverse_iterator       reverse_iterator;
        typedef typename odd_bit_backing_data<uint_t<N>>::const_reverse_iterator const_reverse_iterator;

        typedef typename odd_bit_backing_data<uint_t<N>>::difference_type        difference_type;
        typedef typename odd_bit_backing_data<uint_t<N>>::size_type              size_type;

        typedef          odd_bit_backing_data<uint_t<N>>                         backing_data;
        typedef typename odd_bit_backing_data<uint_t<N>>::internal_data_type     internal_data_type;

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

        inline void resize(size_type n) {
            m_data.resize(n);
        }

        inline void resize(size_type n, const value_type& val) {
            m_data.resize(n, val);
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
