#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/compressors/lz78/LZ78common.hpp>
#include <tudocomp/compressors/lz78/squeeze_node.hpp>

namespace tdc {
namespace lz78 {


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


inline uint8_t bits_hi(uint64_t x) {
	return x == 0 ? 0 : 64 - __builtin_clzll(x);
}

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


template <class Key, class Value,
          class HashFcn = std::hash<Key>,
          class EqualKey = std::equal_to<Key>,
		  class ProbeFcn = QuadraticProber<Key>,
		  class SizeManager = SizeManagerPow2 //SizeManagerPrime //SizeManagerPow2
          >
class MyHash {
	public:
	typedef Key key_t;
	typedef Value value_t;
	typedef std::pair<key_t,value_t> store_t;
	EqualKey m_eq;
	HashFcn m_h;
	ProbeFcn m_probe;
	SizeManager m_sizeman;
	size_t m_size;
	key_t* m_keys;
	value_t* m_values;
	len_t m_entries = 0;
	static constexpr float load_factor = 0.8f;
	static constexpr value_t empty_val = undef_id;
	static constexpr len_t initial_size = 4; // nark::__hsm_stl_next_prime(1)
#ifdef CHECKCOLLISIONS
	size_t collisions = 0;
	size_t old_size = 0;
#endif

	MyHash() : m_size(initial_size), m_keys(new key_t[initial_size]), m_values(new value_t[initial_size]) {
		for(size_t i = 0; i < m_size; ++i) m_values[i] = undef_id;
	}
	~MyHash() {
		delete [] m_keys;
		delete [] m_values;
	}
	void reserve(len_t hint) {
		const auto size = m_sizeman.get_min_size(hint);
		if(m_entries > 0) {
			const size_t oldsize = m_size;
			m_size = size;
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
			m_values = (value_t*) realloc(m_values, sizeof(value_t) * m_size);
			//memset(m_values, 0, sizeof(value_t)*size);
			for(size_t i = 0; i < m_size; ++i) m_values[i] = undef_id;
			m_keys = (key_t*) realloc(m_keys, sizeof(key_t) * m_size);
		}
	}

	class Iterator {
		MyHash* m_table;
		len_t m_offset;
		public:
		Iterator(MyHash* table, len_t offset)
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

	std::pair<Iterator,bool> insert(std::pair<key_t,value_t>&& value) {
		len_t i=0;
//		const size_t _size = table_size()-1;
		//const size_t _size = table_size();
		DCHECK_NE(value.second, empty_val); // cannot insert the empty value
		size_t hash = m_sizeman.mod_tablesize(m_h(value.first), table_size());
		m_probe.init(value.first);
		len_t tablepos = hash;
		while(true) {
			if(m_values[tablepos] == empty_val) {
				m_keys[tablepos] = std::move(value.first);
				m_values[tablepos] = std::move(value.second);
				++m_entries;
				if(tdc_unlikely(table_size()*load_factor < m_entries)) {
					auto toinsert = std::make_pair(m_keys[tablepos], m_values[tablepos]);
					reserve((table_size()-1)<<1);
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
				hash = m_sizeman.mod_tablesize(m_h(value.first), table_size());
				m_probe.init(value.first);
			}

#endif
			++i; //collision
			tablepos = m_sizeman.mod_tablesize(m_probe.template get<SizeManager>(i, tablepos, hash, table_size()), table_size());
			DCHECK_LT(i, table_size()) << "Hash table is full!";
#ifdef CHECKCOLLISIONS
			if(old_size != table_size()) std::cout << "myhash size " << table_size() << std::endl;
			old_size = table_size();
			++collisions;
#endif
		}
	}

};


class MyHashTrie : public Algorithm, public LZ78Trie<factorid_t> {
    using squeeze_node_t = ::tdc::lz78::node_t;
	MyHash<squeeze_node_t,factorid_t,MixHasher,std::equal_to<squeeze_node_t>,LinearProber<squeeze_node_t>> table;

public:
    inline static Meta meta() {
        Meta m("lz78trie", "myhash", "Lempel-Ziv 78 MyHash Trie");
		return m;
	}

    MyHashTrie(Env&& env, factorid_t reserve = 0) : Algorithm(std::move(env)) {
		if(reserve > 0) {
			table.reserve(reserve);
		}
    }

	node_t add_rootnode(uliteral_t c) override {
		table.insert(std::make_pair<squeeze_node_t,factorid_t>(create_node(0, c), size()));
		return size() - 1;
	}

    node_t get_rootnode(uliteral_t c) override {
        return c;
    }

	void clear() override {
//		table.clear();

	}

    node_t find_or_insert(const node_t& parent_w, uliteral_t c) override {
        auto parent = parent_w.id();
        const factorid_t newleaf_id = size(); //! if we add a new node, its index will be equal to the current size of the dictionary

		auto ret = table.insert(std::make_pair(create_node(parent,c), newleaf_id));
		if(ret.second) return undef_id; // added a new node
		return ret.first.value();
    }

    factorid_t size() const override {
        return table.entries();
    }
};

}} //ns

