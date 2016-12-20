#pragma once
#include <tudocomp/def.hpp>
#include <tudocomp/util.hpp>

namespace tdc {

class MixHasher { // https://gist.github.com/badboy/6267743
public:
	size_t operator()(size_t key)
	{
		key = (~key) + (key << 21); // key = (key << 21) - key - 1;
		key = key ^ (key >> 24);
		key = (key + (key << 3)) + (key << 8); // key * 265
		key = key ^ (key >> 14);
		key = (key + (key << 2)) + (key << 4); // key * 21
		key = key ^ (key >> 28);
		key = key + (key << 31);
		return key;
	}
};

struct SizeManagerPow2 {
	static inline len_t get_min_size(const len_t& hint) {
		return 1ULL<<bits_hi(hint); //	or	return nark::__hsm_stl_next_prime(hint);
	}
	static inline len_t mod_tablesize(const len_t& index, const len_t& tablesize) {
		return index & (tablesize-1);
	}
};

template<class key_t>
class QuadraticProber {
	public:
	static inline void init(const key_t&) {
	}
	template<class SizeManager>
	static inline len_t get(const len_t& i, const len_t&, const len_t& init, const len_t&) {
		return init+i*i;
	}
};

template<class key_t>
class GaussProber {
	public:
	static inline void init(const key_t&) {
	}
	template<class SizeManager>
	static inline len_t get(const len_t& i, const len_t& tablepos, const len_t&, const len_t&) {
		return tablepos+i;
	}
};


template<class key_t>
class LinearProber {
	public:
	static inline void init(const key_t&) {
	}
	template<class SizeManager>
	static inline len_t get(const len_t&, const len_t& tablepos, const len_t&, const len_t&) {
		return tablepos+1;
	}
};

template<class SizeManager>
struct _DoubleHashingProber {
	static inline len_t get(const size_t& hash_value, const len_t& max_value) {
		return (1+(hash_value%(max_value-1))) | 1;
	}
};


template<class key_t, class HashFcn>
class DoubleHashingProber {
	HashFcn f;
	size_t hash_value;
	public:
	inline void init(const key_t& key) {
		hash_value = f(key);
	}
	template<class SizeManager>
	inline len_t get(const len_t&, const len_t& tablepos, const len_t&, const len_t& max_value) const {
		//return init+i*hash_value;
		return tablepos+ _DoubleHashingProber<SizeManager>::get(hash_value, max_value); //( (1+(hash_value%(max_value-1)))|1);
	}
};


}
