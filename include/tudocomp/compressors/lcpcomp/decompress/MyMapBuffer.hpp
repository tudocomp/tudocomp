#pragma once

#include <tudocomp/Algorithm.hpp>
#include <tudocomp/def.hpp>
#include <sdsl/int_vector.hpp>
#include <tudocomp/util/Hash.hpp>

namespace tdc {
namespace lcpcomp {


template <class Key, class Value,
          class HashFcn = std::hash<Key>,
          class EqualKey = std::equal_to<Key>,
		  class SizeManager = SizeManagerPow2 ,
	      Value empty_val = std::numeric_limits<Value>::max()
          >
class MyHash {
	public:
	typedef Key key_t;
	typedef Value value_t;
	EqualKey m_eq;
	HashFcn m_h;
	SizeManager m_sizeman;
	size_t m_size;
	key_t* m_keys;
	value_t* m_values;
	sdsl::bit_vector m_removed;
	len_t entries = 0;
	static constexpr float load_factor = 0.8f;
	static constexpr len_t initial_size = 4; // nark::__hsm_stl_next_prime(1)
#ifdef CHECKCOLLISIONS
	size_t collisions = 0;
	size_t old_size = 0;
#endif

	MyHash() : m_size(initial_size), m_keys(new key_t[initial_size]), m_values(new value_t[initial_size]), m_removed(initial_size,0), m_deleted(0) {
		std::fill(m_values, m_values+m_size, empty_val);
		std::fill(m_keys, m_keys+m_size, empty_val);
	}
	~MyHash() {
		delete [] m_keys;
		delete [] m_values;
	}
	void reserve(len_t hint) {
		const auto size = m_sizeman.get_min_size(hint);
		if(entries > 0) {
			const size_t oldsize = m_size;
			m_size = size;
			// key_t* keys = new key_t[m_size];
			// value_t* values = new value_t[m_size];
			key_t* keys = (key_t*) malloc(sizeof(key_t)*m_size);
			value_t* values = (value_t*) malloc(sizeof(value_t)*m_size);
			std::fill(values, values+m_size, empty_val);
			std::fill(keys, keys+m_size, empty_val);
//			for(size_t i = 0; i < m_size; ++i) values[i] = 0;
//			memset(values, 0, sizeof(value_t)*size);
			std::swap(m_values,values);
			std::swap(m_keys,keys);
			sdsl::bit_vector removed(m_size,0);
			removed.swap(m_removed);
			entries=0;
			m_deleted = 0;
			for(len_t i = 0; i < oldsize; ++i) {
				if(removed[i]) continue;
				if(values[i] == empty_val) continue;
				//auto ret =
				insert(std::move(keys[i]), std::move(values[i]));
				//DCHECK_EQ(ret.second, true); // no duplicates
			}
			delete [] keys;
			delete [] values;
		}
		else {
			m_size = size;
			m_values = (value_t*) realloc(m_values, sizeof(value_t) * m_size);
			//memset(m_values, 0, sizeof(value_t)*size);
//			for(size_t i = 0; i < m_size; ++i) m_values[i] = empty_val;
			std::fill(m_values, m_values+m_size, empty_val);
			m_keys = (key_t*) realloc(m_keys, sizeof(key_t) * m_size);
			std::fill(m_keys, m_keys+m_size, empty_val);
			m_removed.resize(m_size); // hopefully filled with zeros
			//m_removed.resize(m_size, 0);
		}
		DCHECK_EQ(m_removed.size(), m_size);
	}

	size_t m_deleted;
	inline len_t size() const { return m_size; }

	std::vector<value_t> find_and_remove(const key_t& key) {
		len_t tablepos = m_sizeman.mod_tablesize(m_h(key), size());
		while(true) {
			if(m_values[tablepos] == empty_val) {
				return std::vector<value_t>();
			}
			if(m_keys[tablepos] > key) {
				return std::vector<value_t>();
			}
			if(m_keys[tablepos] == key) {
//				const size_t oldtablepos = tablepos;
				std::vector<value_t> ret;
				do {
					if(!m_removed[tablepos]) {
						ret.push_back(m_values[tablepos]);
						++m_deleted;
						m_removed[tablepos] = true;
					}
					do {
						tablepos = m_sizeman.mod_tablesize(tablepos+1, size());
					} while(m_keys[tablepos] < key);
				} while(m_keys[tablepos] == key);
				// if(tdc_unlikely( 3*(entries - m_deleted) < size()*load_factor)) { // shrink
				// 	reserve(size()>>1);
				// }
				return ret;
//				return std::vector<value_t>{ m_values + oldtablepos, m_values+tablepos }; //does not work becaus of mod!
			}
			DCHECK_LT(m_keys[tablepos], key);
			tablepos = m_sizeman.mod_tablesize(tablepos+1, size());
		}
		return std::vector<value_t>();
	}

	void insert(key_t key, value_t value) {
		len_t i=0;
		DCHECK_NE(value, empty_val); // cannot insert the empty value
		len_t tablepos = m_sizeman.mod_tablesize(m_h(key), size());
		bool removed = false;
		while(true) {
			if(m_values[tablepos] == empty_val || (m_keys[tablepos] > key && m_removed[tablepos] == true)) {
				m_removed[tablepos] = removed;
				m_keys[tablepos] = std::move(key);
				m_values[tablepos] = std::move(value);
				++entries;
				if(tdc_unlikely(size()*load_factor < entries)) {
					reserve((size()-1)<<1);
				}
				return;// true;
			}
			// if(m_eq(m_keys[tablepos], key)) {
			// 	return;
			// 	//std::make_pair(Iterator(this,tablepos), false);
			// }
//#ifdef USE_KNUTH_ORDERED //we used the reverse ordered of knuth since we insert values in ascending order
			if(m_keys[tablepos] > key) {
				std::swap(key,m_keys[tablepos]); // put value into m_data[tablepos], kicking the stored value in the table out
				std::swap(value,m_values[tablepos]);
				{//std::swap(removed,m_removed[tablepos]);
					bool tmp = m_removed[tablepos];
					m_removed[tablepos] = removed;
					removed = tmp;
				}
				// hash = m_sizeman.mod_tablesize(m_h(key), size());
			}

//#endif
			++i; //collision
			tablepos = m_sizeman.mod_tablesize(tablepos+1, size());
			DCHECK_LT(i, size());// << "Hash table is full!";
#ifdef CHECKCOLLISIONS
			if(old_size != size()) {
                DVLOG(1) << "myhash size " << size() << std::endl;
            }
			old_size = size();
			++collisions;
#endif
		}
	}

};

// template<>
// len_t const MyHash<len_t,len_t>::empty_val {std::numeric_limits<len_t>::max()};



class MyMapBuffer : public Algorithm {
    public:
    inline static Meta meta() {
        Meta m("lcpcomp_dec", "MyMap");
        m.option("lazy").dynamic(0);
        return m;
    }

private:
    std::vector<uliteral_t> m_buffer;
    MyHash<len_t, len_t, MixHasher> m_fwd;
//    std::unordered_multimap<len_t, len_t> m_fwd;
    sdsl::bit_vector m_decoded;

    len_t m_cursor;
    len_t m_longest_chain;
    len_t m_current_chain;

    //storing factors
    std::vector<len_t> m_target_pos;
    std::vector<len_t> m_source_pos;
    std::vector<len_t> m_length;

    const size_t m_lazy; // number of lazy rounds

    inline void decode_literal_at(len_t pos, uliteral_t c) {
        ++m_current_chain;
        m_longest_chain = std::max(m_longest_chain, m_current_chain);

        m_buffer[pos] = c;
        m_decoded[pos] = 1;

        for(len_t it : m_fwd.find_and_remove(pos)) {
            decode_literal_at(it, c); // recursion
		}

        // for(auto it = m_fwd.find(pos); it != m_fwd.end(); it = m_fwd.find(pos)) { //TODO: replace with equal_range!
        //     decode_literal_at(it->second, c); // recursion
        // }
        // m_fwd.erase(pos);

        --m_current_chain;
    }

    inline void decode_lazy_() {
        const len_t factors = m_source_pos.size();
        for(len_t j = 0; j < factors; ++j) {
            const len_t& target_position = m_target_pos[j];
            const len_t& source_position = m_source_pos[j];
            const len_t& factor_length = m_length[j];
            for(len_t i = 0; i < factor_length; ++i) {
                if(m_decoded[source_position+i]) {
                    m_buffer[target_position+i] = m_buffer[source_position+i];
                    m_decoded[target_position+i] = 1;
                }
            }
        }
    }

public:
    inline MyMapBuffer(Env&& env, len_t size)
        : Algorithm(std::move(env)), m_cursor(0), m_longest_chain(0), m_current_chain(0), m_lazy(this->env().option("lazy").as_integer())
    {
//		m_fwd.reserve(20);
        m_buffer.resize(size, 0);
        m_decoded = sdsl::bit_vector(size, 0);
    }

    inline void decode_literal(uliteral_t c) {
        decode_literal_at(m_cursor++, c);
    }

    inline void decode_factor(const len_t source_position, const len_t factor_length) {
        bool factor_stored = false;
        for(len_t i = 0; i < factor_length; ++i) {
            const len_t src_pos = source_position+i;
            if(m_decoded[src_pos]) {
                m_buffer[m_cursor] = m_buffer[src_pos];
                m_decoded[m_cursor] = 1;
            }
            else if(factor_stored == false) {
                factor_stored = true;
                m_target_pos.push_back(m_cursor);
                m_source_pos.push_back(src_pos);
                m_length.push_back(factor_length-i);
            }
            ++m_cursor;
        }
    }

    inline void decode_lazy() {
        size_t lazy = m_lazy;
        while(lazy > 0) {
            decode_lazy_();
            --lazy;
        }
    }

    inline void decode_eagerly() {
        const len_t factors = m_source_pos.size();
		env().log_stat("factors", factors);
        env().begin_stat_phase("Decoding Factors");
        for(len_t j = 0; j < factors; ++j) {
            const len_t& target_position = m_target_pos[j];
            const len_t& source_position = m_source_pos[j];
            const len_t& factor_length = m_length[j];
            for(len_t i = 0; i < factor_length; ++i) {
                if(m_decoded[source_position+i]) {
                    decode_literal_at(target_position+i, m_buffer[source_position+i]);
                } else {
                    m_fwd.insert(source_position+i, target_position+i);
                }
            }

            IF_STATS({
			    if((j+1) % (factors/5) == 0 ) {
				    env().log_stat("hash table size", m_fwd.size());
				    env().log_stat("hash table entries", m_fwd.entries);
				    env().log_stat("hash table deletions", m_fwd.m_deleted);
				    env().end_stat_phase();
            		env().begin_stat_phase("Decoding Factors at position " + std::to_string(target_position));
			    }
            })
        }
		env().end_stat_phase();
    }



    inline len_t longest_chain() const {
        return m_longest_chain;
    }

    inline void write_to(std::ostream& out) {
        for(auto c : m_buffer) out << c;
    }
};


}}//ns

