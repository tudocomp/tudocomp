#ifndef _SDSLEX_GROWABLE_INT_VECTOR_HPP
#define _SDSLEX_GROWABLE_INT_VECTOR_HPP

#include <sdsl/int_vector.hpp>
#include <sdsl/bits.hpp>
#include <sdsl/util.hpp>

namespace sdslex {

class GrowableIntVector {
public:
    typedef sdsl::int_vector<0>::iterator iterator;
    typedef sdsl::int_vector<0>::const_iterator const_iterator;

    typedef sdsl::int_vector<0>::reference reference;
    typedef sdsl::int_vector<0>::const_reference const_reference;

    typedef sdsl::int_vector<0>::size_type size_type;
    typedef sdsl::int_vector<0>::difference_type difference_type;

private:
    sdsl::int_vector<0> vec;
    size_type m_size;

public:
    inline GrowableIntVector(): vec(0,0,0), m_size(0) {
    }

    inline void push_back(uint64_t value) {
        uint64_t value_width = sdsl::bits::hi(value) + 1;

        size_type old_cap = vec.size();

        //DLOG(INFO) << "value = " << value;
        //DLOG(INFO) << "value_width = " << value_width;
        //DLOG(INFO) << "vec.width() = " << int(vec.width());
        //DLOG(INFO) << "vec.size() = " << int(vec.size());

        // grow width if neccessary
        if (value_width > vec.width()) {
            sdsl::util::expand_width(vec, value_width);

            DLOG(INFO) << "new vec.width() = " << int(vec.width());
        }

        // grow capacity if neccessary (vec.size() is the capacity)
        if (m_size == vec.size()) {
            // Needs cast to signed m_size in the middle so that it properly
            // calculates -1 for the m_size == 0 case.
            size_type cap = m_size + (difference_type(m_size) - 1) / 2 + 1;
            vec.resize(cap);

            DLOG(INFO) << "new vec.size() = " << int(vec.size());
        }

        DCHECK(old_cap <= vec.size());

        vec[m_size] = std::move(value);
        m_size++;
    }

    inline const iterator begin() {
        return vec.begin();
    }

    inline const iterator end() {
        return vec.end() - (vec.size() - m_size);
    }

    inline const const_iterator begin() const {
        return vec.begin();
    }

    inline const const_iterator end() const {
        return vec.end() - (vec.size() - m_size);
    }

    inline size_type size() const {
        return m_size;
    }

    inline auto operator[](const size_type& idx) -> reference {
        reference tmp = vec[idx];
        return tmp;
    }

    inline auto operator[](const size_type& idx) const -> const_reference
    {
        const_reference tmp = vec[idx];
        return tmp;
    }
};

}

#endif
