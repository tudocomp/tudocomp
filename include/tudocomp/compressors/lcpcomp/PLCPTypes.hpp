#pragma once
#include <stxxl/bits/containers/vector.h>

namespace tdc { namespace lcpcomp {
	
	
constexpr uint64_t blockSize = 2 * 1024 * 1024; // 2MiB per block
constexpr uint64_t pageSize = 4;                // 4 blocks per page
constexpr uint64_t cachedPages = 8;             // 8 cached pages (will be adjusted later)

template <typename unsigned_t>
struct literal_t {
 public:
    unsigned_t copyTo;
    char character;
    literal_t() {}        
    literal_t(unsigned_t copyTo, char character) {
        this->copyTo = copyTo;
        this->character = character;
    }
};

template <typename unsigned_t>
struct factor_t {
 public:
    unsigned_t copyFrom, length;
    factor_t() {}        
    factor_t(unsigned_t copyFrom, unsigned_t length) {
        this->copyFrom = copyFrom;
        this->length = length;
    }
    
    unsigned_t copyFromEnd() const {
        return copyFrom + length;
    }
};

template <typename unsigned_t>
struct reference_t : public factor_t<unsigned_t> {
    unsigned_t copyTo;
    reference_t() {}        
    reference_t(unsigned_t copyTo, unsigned_t copyFrom, unsigned_t length) 
            : factor_t<unsigned_t>(copyFrom, length) {
        this->copyTo = copyTo;
    }
    void removePrefix(unsigned_t prefixLength) {
        copyTo += prefixLength;
        factor_t<unsigned_t>::copyFrom += prefixLength;
        factor_t<unsigned_t>::length -= prefixLength;
    }
    
    unsigned_t copyToEnd() const {
        return copyTo + factor_t<unsigned_t>::length;
    }
    
    template<typename unsigned_other_t>
    reference_t<unsigned_other_t> convert() {
        return reference_t<unsigned_other_t>(
            uint64_t(copyTo), 
            uint64_t(factor_t<unsigned_t>::copyFrom), 
            uint64_t(factor_t<unsigned_t>::length));
    }
};


typedef uint_t<40> unsigned_initial_t;
typedef reference_t<unsigned_initial_t> ref_initial_t;
typedef typename stxxl::VECTOR_GENERATOR<ref_initial_t, pageSize, cachedPages, blockSize>::result vector_of_ref_initial_t;
typedef typename vector_of_ref_initial_t::bufwriter_type vector_of_ref_writer_initial_t;
typedef typename vector_of_ref_initial_t::bufreader_type vector_of_ref_reader_initial_t;
typedef typename stxxl::VECTOR_GENERATOR<char, pageSize, cachedPages, blockSize>::result vector_of_char_t;
typedef typename vector_of_char_t::bufwriter_type vector_of_char_writer_t;
typedef typename vector_of_char_t::bufreader_type vector_of_char_reader_t;


template <typename unsigned_t>
struct reference_limits_t {
	const unsigned_t min_unsigned = std::numeric_limits<unsigned_t>::min();
	const unsigned_t max_unsigned = std::numeric_limits<unsigned_t>::max();
	const reference_t<unsigned_t> min_factor = reference_t<unsigned_t>(min_unsigned, min_unsigned, min_unsigned);
	const reference_t<unsigned_t> max_factor = reference_t<unsigned_t>(max_unsigned, max_unsigned, max_unsigned);
	reference_t<unsigned_t> min_value() const { return reference_limits_t::min_factor; }
	reference_t<unsigned_t> max_value() const { return reference_limits_t::max_factor; }
};

template <typename unsigned_t>
struct compare_by_copyTo_t : reference_limits_t<unsigned_t> {
	bool operator()(const reference_t<unsigned_t> &a, const reference_t<unsigned_t> &b) const 
	{ return a.copyTo < b.copyTo; }
};

template <typename unsigned_t>
struct compare_by_copyFrom_t : reference_limits_t<unsigned_t> {
	bool operator()(const reference_t<unsigned_t> &a, const reference_t<unsigned_t> &b) const 
	{ return a.copyFrom < b.copyFrom; }
};

template <typename unsigned_t>
struct literal_limits_t {
	const unsigned_t min_unsigned = std::numeric_limits<unsigned_t>::min();
	const unsigned_t max_unsigned = std::numeric_limits<unsigned_t>::max();
	const literal_t<unsigned_t> min_literal = literal_t<unsigned_t>(min_unsigned, '\0');
	const literal_t<unsigned_t> max_literal = literal_t<unsigned_t>(max_unsigned, '\0');
	literal_t<unsigned_t> min_value() const { return literal_limits_t::min_literal; }
	literal_t<unsigned_t> max_value() const { return literal_limits_t::max_literal; }
};

template <typename unsigned_t>
struct compare_by_copyTo_lit_t : literal_limits_t<unsigned_t> {
	bool operator()(const literal_t<unsigned_t> &a, const literal_t<unsigned_t> &b) const 
	{ return a.copyTo < b.copyTo; }
};

}} //ns
