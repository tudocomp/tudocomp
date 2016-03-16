#ifndef _SDSLEX_INT_VECTOR_WRAPPER_HPP
#define _SDSLEX_INT_VECTOR_WRAPPER_HPP

#include <sdsl/int_vector.hpp>

namespace sdslex {

class int_vector_wrapper {

private:
    sdsl::int_vector<8>::size_type m_len;
    uint64_t* m_ptr;
    sdsl::int_vector<8>::int_width_type m_width;

public:
    inline int_vector_wrapper(const uint8_t* data, size_t len)
        : m_len(8 * len), m_ptr((uint64_t*)data), m_width(8) {
    }

    const sdsl::int_vector<8>& int_vector = *((sdsl::int_vector<8>*)this);
};

}

#endif

