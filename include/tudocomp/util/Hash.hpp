#pragma once

#include <tudocomp/def.hpp>
#include <tudocomp/util.hpp>
#include <tudocomp/Algorithm.hpp>

#include <tudocomp_stat/StatPhase.hpp>
// #include <tudocomp/util/hash/clhash.h>
// #include <tudocomp/util/hash/zobrist.h>

namespace tdc {

	//TODO: can operator() be made to constexpr?
struct VignaHasher : public Algorithm { // http://xorshift.di.unimi.it/splitmix64.c
    inline static Meta meta() {
        Meta m("hash_function", "vigna", "Vigna's splitmix Hasher");
		return m;
	}
	VignaHasher(Env&& env) : Algorithm(std::move(env)) {}
	inline uint64_t operator()(uint64_t x) const {
		x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
		x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
		x = x ^ (x >> 31);
		return x;
	}
};
struct _VignaHasher  { // http://xorshift.di.unimi.it/splitmix64.c
	inline uint64_t operator()(uint64_t x) const {
		x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
		x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
		x = x ^ (x >> 31);
		return x;
	}
};

struct KnuthHasher : public Algorithm { // http://xorshift.di.unimi.it/splitmix64.c
    inline static Meta meta() {
        Meta m("hash_function", "knuth", "Knuth's splitmix Hasher");
		return m;
	}
	KnuthHasher(Env&& env) : Algorithm(std::move(env)) {}
	size_t operator()(size_t key)
	{
		return key * 2654435769ULL;
	}
};


struct MixHasher : public Algorithm { // https://gist.github.com/badboy/6267743
    inline static Meta meta() {
        Meta m("hash_function", "mixer", "MixHasher");
		return m;
	}
	MixHasher(Env&& env) : Algorithm(std::move(env)) {}
	size_t operator()(size_t key) const {
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

struct NoopHasher : public Algorithm {
    inline static Meta meta() {
        Meta m("hash_function", "noop", "Identity");
		return m;
	}
	NoopHasher(Env&& env) : Algorithm(std::move(env)) {}
	size_t operator()(size_t key) const {
		return key;
	}
};


//// Hash Table Size Managers

struct SizeManagerPow2 : public Algorithm {
    inline static Meta meta() {
        Meta m("hash_manager", "pow", "Pow2 Hash Table Sizes");
		return m;
	}
	SizeManagerPow2(Env&& env) : Algorithm(std::move(env)) {}
	void resize(const len_t) {
	}
	/**
	 * The lowest possible size larger than hint
	 */
	static inline len_t get_min_size(const len_t& hint) {
		return 1ULL<<bits_hi(std::max<len_t>(hint,3UL));
	}
	/**
	 *  Compute index % tablesize
	 *  Since tablesize is a power of two, a bitwise-AND is equivalent and faster
	 */
	static inline len_t mod_tablesize(const size_t& index, const len_t& tablesize, const size_t& , const size_t&) {
		return index & (tablesize-1);
	}
};



struct SizeManagerDirect : public Algorithm {
    inline static Meta meta() {
        Meta m("hash_manager", "direct", "Direct Hash Table Sizes");
		return m;
	}
	SizeManagerDirect(Env&& env) : Algorithm(std::move(env)) {}
	/**
	 * The lowest possible size larger than hint
	 */
	static inline len_t get_min_size(const len_t& hint) {
		return std::max<len_t>(hint,3UL);
	}
	void resize(const len_t) {
	}

	/**
	 *  Compute index % tablesize
	 *  Since tablesize is a power of two, a bitwise-AND is equivalent and faster
	 *  from http://www.idryman.org/blog/2017/05/03/writing-a-damn-fast-hash-table-with-tiny-memory-footprints/
	 */
	inline len_t mod_tablesize(const size_t, const len_t tablesize, const size_t key, const size_t probe) {
		const __uint128_t v = (static_cast<uint64_t>(key + (static_cast<__uint128_t>(probe) << 64)/(tablesize))) * static_cast<__uint128_t>(tablesize);
		const len_t ret = v >> 64;
		DCHECK_LT(ret, tablesize);
		return ret;
		// return index % tablesize; // fallback
	}
};

struct SizeManagerNoob : public Algorithm {
    inline static Meta meta() {
        Meta m("hash_manager", "noob", "Classic Modulo Reduction");
		return m;
	}
	SizeManagerNoob(Env&& env) : Algorithm(std::move(env)) {}
	/**
	 * The lowest possible size larger than hint
	 */
	static inline len_t get_min_size(const len_t& hint) {
		return std::max<len_t>(hint,3UL);
	}
	void resize(const len_t) {
	}

	/**
	 *  Compute index % tablesize
	 */
	inline len_t mod_tablesize(const size_t index, const len_t tablesize, const size_t , const size_t ) {
		return index % tablesize; // fallback
	}
};


struct SizeManagerPrime : public Algorithm {
    inline static Meta meta() {
        Meta m("hash_manager", "prime", "Prime Hash Table Sizes");
		return m;
	}
	SizeManagerPrime(Env&& env) : Algorithm(std::move(env)) {}

	static inline len_t get_min_size(const len_t& hint) {
		return next_prime(hint);
	}
	void resize(const len_t) {
	}
	static inline len_t mod_tablesize(const size_t& index, const len_t& tablesize, const size_t& , const size_t&) {
		//return (static_cast<uint64_t>(index)*static_cast<uint64_t>(tablesize)) >> 32;
		return index % tablesize;
	}
	// From https://github.com/rockeet/nark-hashmap
	// By rockeet
	// GNU Affero General Public License
	inline static size_t next_prime(size_t __n)
	{
		static const size_t primes[] =
		{
			5,11,19,37,   53ul,         97ul,         193ul,       389ul,
			769ul,        1543ul,       3079ul,       6151ul,      12289ul,
			24593ul,      49157ul,      98317ul,      196613ul,    393241ul,
			786433ul,     1572869ul,    3145739ul,    6291469ul,   12582917ul,
			25165843ul,   50331653ul,   100663319ul,  201326611ul, 402653189ul,
			805306457ul,  1610612741ul, 3221225473ul, 4294967291ul,
			/* 30    */ (size_t)8589934583ull,
			/* 31    */ (size_t)17179869143ull,
			/* 32    */ (size_t)34359738337ull,
			/* 33    */ (size_t)68719476731ull,
			/* 34    */ (size_t)137438953447ull,
			/* 35    */ (size_t)274877906899ull,
			/* 36    */ (size_t)549755813881ull,
			/* 37    */ (size_t)1099511627689ull,
			/* 38    */ (size_t)2199023255531ull,
			/* 39    */ (size_t)4398046511093ull,
			/* 40    */ (size_t)8796093022151ull,
			/* 41    */ (size_t)17592186044399ull,
			/* 42    */ (size_t)35184372088777ull,
			/* 43    */ (size_t)70368744177643ull,
			/* 44    */ (size_t)140737488355213ull,
			/* 45    */ (size_t)281474976710597ull,
			/* 46    */ (size_t)562949953421231ull,
			/* 47    */ (size_t)1125899906842597ull,
			/* 48    */ (size_t)2251799813685119ull,
			/* 49    */ (size_t)4503599627370449ull,
			/* 50    */ (size_t)9007199254740881ull,
			/* 51    */ (size_t)18014398509481951ull,
			/* 52    */ (size_t)36028797018963913ull,
			/* 53    */ (size_t)72057594037927931ull,
			/* 54    */ (size_t)144115188075855859ull,
			/* 55    */ (size_t)288230376151711717ull,
			/* 56    */ (size_t)576460752303423433ull,
			/* 57    */ (size_t)1152921504606846883ull,
			/* 58    */ (size_t)2305843009213693951ull,
			/* 59    */ (size_t)4611686018427387847ull,
			/* 60    */ (size_t)9223372036854775783ull,
			/* 61    */ (size_t)18446744073709551557ull,
		};
		const size_t* __first = primes;
		const size_t* __last = primes + sizeof(primes)/sizeof(primes[0]);
		const size_t* pos = std::lower_bound(__first, __last, __n);
		return pos == __last ? __last[-1] : *pos;
	}
};

//// Collision Prober

struct QuadraticProber : public Algorithm {
    inline static Meta meta() {
        Meta m("hash_prober", "quad", "Quadratic Prober");
		return m;
	}
	QuadraticProber(Env&& env) : Algorithm(std::move(env)) {}
	template<class key_t>
	static inline void init(const key_t&) {
	}
	/**
	 * i: number of collisions
	 * tablepos: at which position we are currently in the hash table
	 * init: the initial hash value (mod table size)
	 * table_size: the size of the table
	 */
	template<class SizeManager>
	static inline len_t get(const len_t& i, const len_t&, const len_t& init, const len_t&) {
		return init+i*i;
	}
};

struct GaussProber : public Algorithm {
    inline static Meta meta() {
        Meta m("hash_prober", "gauss", "Gauss Prober");
		return m;
	}
	GaussProber(Env&& env) : Algorithm(std::move(env)) {}
	template<class key_t>
	static inline void init(const key_t&) {
	}
	template<class SizeManager>
	static inline len_t get(const len_t& i, const len_t& tablepos, const len_t&, const len_t&) {
		return tablepos+i;
	}
};


struct LinearProber : public Algorithm {
    inline static Meta meta() {
        Meta m("hash_prober", "linear", "Linear Prober");
		return m;
	}
	LinearProber(Env&& env) : Algorithm(std::move(env)) {}

	template<class key_t>
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

//// Rolling Hash Functions

class ZBackupRollingHash : public Algorithm {
	public:
	typedef uint64_t key_type;
	private:
	key_type m_val=0;
	key_type m_len=0;
	public:
    inline static Meta meta() {
        Meta m("hash_roller", "zbackup", "ZBackup Rolling Hash");
		return m;
	}
	ZBackupRollingHash(Env&& env) : Algorithm(std::move(env)) {}
	void operator+=(char c) {
		m_val = (m_val << (sizeof(char)*sizeof(key_type))) + m_val + static_cast<key_type>(c); // % 18446744073709551557ULL;
		m_len += m_len << 8;
	}
	key_type operator()() {
		return (m_val+m_len) % (-1); //18446744073709551557ULL;
	}
	void clear() {
		m_val=0;
		m_len=0;
	}
};

class WordpackRollingHash : public Algorithm {
	public:
	typedef uint64_t key_type;
	private:
	key_type m_val=0;
	public:
    inline static Meta meta() {
        Meta m("hash_roller", "wordpack", "Wordpacking Rolling Hash");
		return m;
	}
	WordpackRollingHash(Env&& env) : Algorithm(std::move(env)) {}
	void operator+=(char c) {
		m_val = ((m_val + (m_val << 8)) + c);// % 138350580553ULL;   // prime number between 2**63 and 2**64
	}
	key_type operator()() {
		return m_val;
	}
	void clear() {
		m_val=0;
	}
};


template <class Key, class Value,
		  Value undef_id,
          class HashFcn = MixHasher,
          class EqualKey = std::equal_to<Key>,
		  class ProbeFcn = QuadraticProber,
		  class SizeManager = SizeManagerPow2
          >
class HashMap {
	//static_assert(!std::is_same<ProbeFcn, QuadraticProber>::value || !std::is_same<SizeManager,SizeManagerPow2>::value, "Cannot use QuadraticProber with SizeManagerPow2");
	public:
	typedef Key key_t;
	typedef Value value_t;
	typedef std::pair<key_t,value_t> store_t;

    inline static Meta meta() {
        Meta m("hash", "ctable", "Custom Hash Table");
		return m;
	}


	EqualKey m_eq;
	HashFcn m_h;
	ProbeFcn m_probe;
	SizeManager m_sizeman;
	size_t m_size;
	key_t* m_keys;
	value_t* m_values;
	len_t m_entries = 0;
	float m_load_factor = 0.3f;
	static constexpr value_t empty_val = undef_id;
	static constexpr len_t initial_size = 4; // nark::__hsm_stl_next_prime(1)
	IF_STATS(
		size_t m_collisions = 0;
		size_t m_old_size = 0;
		size_t m_resizes = 0;
		size_t m_specialresizes = 0;
	)

	public:
	IF_STATS(
		size_t collisions() const { return m_collisions; }
		void collect_stats(Env&) const {
		StatPhase::log("collisions", collisions());
		StatPhase::log("table size", table_size());
		StatPhase::log("load factor", max_load_factor());
		StatPhase::log("entries", entries());
		StatPhase::log("resizes", m_resizes);
		StatPhase::log("special resizes", m_specialresizes);
		StatPhase::log("load ratio", entries()*100/table_size());
		}
	);
	void max_load_factor(float z) {
		m_load_factor = z;
	}
	float max_load_factor() const noexcept {
		return m_load_factor;
	}
	const size_t m_n;
	const size_t& m_remaining_characters;

	HashMap(Env&, const size_t n, const size_t& remaining_characters)
		: m_h(create_env(HashFcn::meta()))
		, m_probe(create_env(ProbeFcn::meta()))
// env.env_for_option("hash_prober"))
		, m_sizeman(create_env(SizeManager::meta())) //env.env_for_option("hash_manager"))
		, m_size(initial_size)
		, m_keys(new key_t[initial_size])
		, m_values(new value_t[initial_size])
		, m_n(n)
		, m_remaining_characters(remaining_characters)
	{
		for(size_t i = 0; i < m_size; ++i) m_values[i] = undef_id;
		m_sizeman.resize(m_size);
	}
	template<class T>
	void incorporate(T&& o, len_t newsize)
	{
		if(m_keys != nullptr) { delete [] m_keys; }
		if(m_values != nullptr) { delete [] m_values; }

		m_keys = std::move(o.m_keys);
		m_values = std::move(o.m_values);
		m_size = std::move(o.m_size);
		m_sizeman.resize(m_size);
		m_entries = std::move(o.m_entries);
		o.m_keys = nullptr;
		o.m_values = nullptr;
	IF_STATS(
		m_collisions     = ( o.m_collisions);
		m_old_size       = ( o.m_old_size);
		m_resizes        = ( o.m_resizes);
		m_specialresizes = ( m_specialresizes);
	)
		reserve(newsize);
	}

	MoveGuard m_guard;
	~HashMap() {
        if (m_guard) {
            if(m_keys != nullptr) { delete [] m_keys; }
            if(m_values != nullptr) { delete [] m_values; }
        }
	}
	inline HashMap(HashMap&& other) = default;
    inline HashMap& operator=(HashMap&& other) = default;

	void reserve(len_t hint) {
		IF_STATS(++m_resizes);
		const auto size = m_sizeman.get_min_size(hint);
		if(m_entries > 0) {
			const size_t oldsize = m_size;
			m_size = size;
			m_sizeman.resize(m_size);
			// key_t* keys = new key_t[m_size];
			// value_t* values = new value_t[m_size];
			key_t* keys = (key_t*) malloc(sizeof(key_t)*m_size);
			value_t* values = (value_t*) malloc(sizeof(value_t)*m_size);
			for(size_t i = 0; i < m_size; ++i) values[i] = undef_id;
//			memset(values, 0, sizeof(value_t)*size);
			std::swap(m_values,values);
			std::swap(m_keys,keys);
			m_entries=0;
			for(len_t i = 0; i < oldsize; ++i) {
				if(values[i] == empty_val) continue;
				auto ret = insert(std::make_pair(std::move(keys[i]),std::move(values[i])));
				DCHECK_EQ(ret.second, true); // no duplicates
			}
			delete [] keys;
			delete [] values;
		}
		else {
			m_size = size;
			m_sizeman.resize(m_size);
			m_values = (value_t*) realloc(m_values, sizeof(value_t) * m_size);
			//memset(m_values, 0, sizeof(value_t)*size);
			for(size_t i = 0; i < m_size; ++i) m_values[i] = undef_id;
			m_keys = (key_t*) realloc(m_keys, sizeof(key_t) * m_size);
		}
	}

	class Iterator {
		HashMap* m_table;
		len_t m_offset;
		public:
		Iterator(HashMap* table, len_t offset)
			: m_table(table), m_offset(offset)
		{
		}
		bool operator==(const Iterator& o) const {
			return m_table == o.m_table && m_offset = o.m_offset;
		}
		Iterator& operator=(const Iterator& o) {
			m_table = o.m_table;
			m_offset = o.m_offset;
			return *this;
		}
		const value_t& value() const {
			return m_table->m_values[m_offset];
		}
	};

	inline len_t entries() const { return m_entries; }
	inline len_t table_size() const { return m_size; }
	inline len_t empty() const { return m_entries == 0; }

	std::pair<Iterator,bool> insert(std::pair<key_t,value_t>&& value) {
		len_t i=0;
//		const size_t _size = table_size()-1;
		//const size_t _size = table_size();
		DCHECK_NE(value.second, empty_val); // cannot insert the empty value
		const size_t init_hashvalue = m_h(value.first);
		size_t hash = m_sizeman.mod_tablesize(init_hashvalue, table_size(), init_hashvalue, i);
		m_probe.init(value.first);
		len_t tablepos = hash;
		while(true) {
			if(m_values[tablepos] == empty_val) {
				m_keys[tablepos] = std::move(value.first);
				m_values[tablepos] = std::move(value.second);
				++m_entries;
				if(tdc_unlikely(table_size()*max_load_factor() < m_entries)) {
					auto toinsert = std::make_pair(m_keys[tablepos], m_values[tablepos]);

					size_t expected_size =
					std::is_same<SizeManager,SizeManagerDirect>::value ?
					(m_entries + 3.0/2.0*lz78_expected_number_of_remaining_elements(entries(),m_n,m_remaining_characters))/0.95 :
					(m_entries + lz78_expected_number_of_remaining_elements(entries(),m_n,m_remaining_characters))/0.95;
					expected_size = std::max<size_t>(expected_size, table_size()*1.1);
					if(expected_size < table_size()*2.0*0.95) {
							max_load_factor(0.95f);
						if(std::is_same<SizeManager,SizeManagerDirect>::value) {
							reserve(expected_size);
						} else {
							reserve(expected_size); //(m_entries + lz78_expected_number_of_remaining_elements(entries(),m_n,m_remaining_characters))/0.95);
						}

						IF_STATS(++m_specialresizes);
					} else {
					reserve((table_size()-1)<<1);
					}
					auto ret = insert(std::move(toinsert)); //rehashing invalidates the above computed hash!
					DCHECK_EQ(ret.second, false); // it should now be inserted
					return std::make_pair(ret.first, true);
				}
				return std::make_pair(Iterator(this,tablepos),true);
			}
			if(m_eq(m_keys[tablepos], value.first)) {
				return std::make_pair(Iterator(this,tablepos), false);
			}
#ifdef USE_KNUTH_ORDERED //we used the reverse ordered of knuth since we insert values in ascending order
			if(m_keys[tablepos] > value.first) {
				std::swap(value.first,m_keys[tablepos]); // put value into m_data[tablepos], kicking the stored value in the table out
				std::swap(value.second,m_values[tablepos]);
				hash = m_sizeman.mod_tablesize(init_hashvalue, table_size(), init_hashvalue, i);
				m_probe.init(value.first);
			}

#endif
			++i; //collision
			tablepos = m_sizeman.mod_tablesize(m_probe.template get<SizeManager>(i, tablepos, hash, table_size()), table_size(), init_hashvalue, i);
			DCHECK_LT(i, table_size()) << "Hash table is full!";
			IF_STATS(++m_collisions);
			IF_STATS(IF_DEBUG(
			m_old_size = table_size();
			if(m_old_size != table_size()) {
                DVLOG(1) << "HashMap size " << table_size() << std::endl;
            }))
		}
	}

};




}
