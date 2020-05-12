#pragma once

#include <vector>
#include <tudocomp/compressors/lz78/LZ78Trie.hpp>
#include <tudocomp/Algorithm.hpp>
#include <tudocomp_stat/StatPhase.hpp>
#include <separate/dcheck.hpp>

// #include <tudocomp/util/bits.hpp>

namespace tdc {
namespace lz78 {

template<class T>
class BitwiseVector
{
	public:
	BitwiseVector() {
		std::fill(m_data, m_data+m_datasize, nullptr);
		std::fill(m_lengths, m_lengths+m_datasize, 0);
	}
	~BitwiseVector() { clear(); }

	size_t m_elements = 0;
	constexpr static size_t m_datasize = std::numeric_limits<factorid_t>::digits-1;
	T* m_data[m_datasize];
	size_t m_lengths[m_datasize];
	ON_DEBUG(std::vector<T> m_vector);
	constexpr static uint_fast8_t m_smallbits = 1;
	T m_smalldata[1ULL<<m_smallbits];
	void reserve(size_t) {
	}
	void clear() {
		ON_DEBUG(m_vector.clear());
		m_elements = 0;
		for(size_t i = 0; i < m_datasize; ++i) {
			if(m_data[i] != nullptr) {
				free(m_data[i]);
			}
		}
		std::fill(m_data, m_data+m_datasize, nullptr);
		std::fill(m_lengths, m_lengths+m_datasize, 0);
	}
	size_t size() const { 
		DDCHECK_EQ(m_vector.size(), m_elements);
		return m_elements;
	}

	size_t capacity() const { return size(); }

	void push_back(const T& o) {
		if(size() < 1ULL<<m_smallbits) {
			m_smalldata[m_elements] = o;
		}
		else {
			const uint_fast8_t bitwidth = bits_for(size());
			DDCHECK_LE(m_smallbits, bitwidth);
			const uint_fast8_t dataindex = bitwidth - m_smallbits;
			if(m_data[dataindex] == nullptr) {
				m_data[dataindex] = reinterpret_cast<T*>(malloc(sizeof(T)*(1ULL<<(bitwidth-1))));
			}
			DDCHECK_LT(m_lengths[dataindex], (1ULL<<(bitwidth-1)));
			DDCHECK_LT(dataindex, m_datasize);
			m_data[dataindex][m_lengths[dataindex]++] = o;
		}
		++m_elements;
		ON_DEBUG(m_vector.push_back(o));
		
		// if(bits_for(size()) < m_smallbits) { return m_vector.push_back(o);}
		// const size_t index = bits_for(size()) - m_smallbits;
		// if(m_data[index] == nullptr) {
		// 	m_data[index] = malloc(sizeof(T) * (1ULL<<bits_for(size())));
		// }
		// m_data[index][m_lengths[index]++] = o;
	}
	void set(size_t index, const T& value) {
		if(index < 2) {
			m_smalldata[index] = value;
			ON_DEBUG(m_vector[index] = value);
		} else {
			const uint_fast8_t bitwidth = bits_for(index);
			DDCHECK_LE(m_smallbits, bitwidth);
			const uint_fast8_t dataindex = bitwidth - m_smallbits;
			DDCHECK_LT(dataindex, m_datasize);
			DDCHECK(m_data[dataindex] != nullptr);
			const size_t remaining_index = index - (1ULL<<(bitwidth-1));
			DDCHECK_LT(remaining_index, m_lengths[dataindex]);
			m_data[dataindex][remaining_index] = value;
			ON_DEBUG(m_vector[index] = value);
			DDCHECK_EQ(m_data[dataindex][remaining_index], this->operator[](index));
		}
	}
	const T& operator[](size_t index) const {
		if(index < 2) {
			DDCHECK_EQ(m_smalldata[index], m_vector[index]);
			return m_smalldata[index];
		} else {
			const uint_fast8_t bitwidth = bits_for(index);
			DDCHECK_LE(m_smallbits, bitwidth);
			const uint_fast8_t dataindex = bitwidth - m_smallbits;
			DDCHECK_LT(dataindex, m_datasize);
			DDCHECK(m_data[dataindex] != nullptr);
			const size_t remaining_index = index - (1ULL<<(bitwidth-1));
			DDCHECK_LT(remaining_index, m_lengths[dataindex]);
			DDCHECK_EQ(m_data[dataindex][remaining_index], m_vector[index]);
			return m_data[dataindex][remaining_index];
		}

		// if(bits_for(index) < m_smallbits) { m_vector[index]; }

	}
};


class BinaryKTrie : public Algorithm, public LZ78Trie<> {

    /*
     * The trie is not stored in standard form. Each node stores the pointer to its first child and a pointer to its next sibling (first as first come first served)
     */
    BitwiseVector<factorid_t> m_first_child;
    BitwiseVector<factorid_t> m_next_sibling;
    BitwiseVector<uliteral_t> m_literal;

    IF_STATS(
        size_t m_resizes = 0;
        size_t m_specialresizes = 0;
    )
public:
    inline static Meta meta() {
        Meta m(lz78_trie_type(), "binaryk", "Lempel-Ziv 78 key-Binary Tries");
        return m;
    }

    inline BinaryKTrie(Config&& cfg, size_t n, const size_t& remaining_characters, factorid_t reserve = 0)
    : Algorithm(std::move(cfg))
    , LZ78Trie(n,remaining_characters)
    {
        if(reserve > 0) {
            m_first_child.reserve(reserve);
            m_next_sibling.reserve(reserve);
            m_literal.reserve(reserve);
        }
    }

    IF_STATS(
        MoveGuard m_guard;
        inline ~BinaryKTrie() {
            if (m_guard) {
                StatPhase::log("resizes", m_resizes);
                StatPhase::log("special resizes", m_specialresizes);
                StatPhase::log("table size", m_first_child.capacity());
                StatPhase::log("load ratio", m_first_child.size()*100/m_first_child.capacity());
            }
        }
    )
    BinaryKTrie(BinaryKTrie&& other) = default;
    BinaryKTrie& operator=(BinaryKTrie&& other) = default;

    inline node_t add_rootnode(uliteral_t c) {
        DDCHECK_EQ(c, size());
        m_first_child.push_back(undef_id);
        m_next_sibling.push_back(undef_id);
        m_literal.push_back(c);
        return node_t(c, true);
    }

    inline node_t get_rootnode(uliteral_t c) const {
        return node_t(c, false);
    }

    inline void clear() {
        m_first_child.clear();
        m_next_sibling.clear();
        m_literal.clear();
    }

    inline node_t find_or_insert(const node_t& parent_w, uliteral_t c) {
        auto parent = parent_w.id();
        const factorid_t newleaf_id = size(); //! if we add a new node, its index will be equal to the current size of the dictionary

        DDCHECK_LT(parent, size());


        if(m_first_child[parent] == undef_id) {
            m_first_child.set(parent, newleaf_id);
        } else {
            factorid_t node = m_first_child[parent];
            while(true) { // search the binary tree stored in parent (following left/right siblings)
                if(c == m_literal[node]) return node_t(node, false);
                if(m_next_sibling[node] == undef_id) {
                    m_next_sibling.set(node, newleaf_id);
                    break;
                }
                node = m_next_sibling[node];
            }
        }
        if(m_first_child.capacity() == m_first_child.size()) {
            const size_t newbound =    m_first_child.size()+expected_number_of_remaining_elements(size());
            if(newbound < m_first_child.size()*2 ) {
                m_first_child.reserve   (newbound);
                m_next_sibling.reserve  (newbound);
                m_literal.reserve       (newbound);
                IF_STATS(++m_specialresizes);
            }
            IF_STATS(++m_resizes);
        }
        m_first_child.push_back(undef_id);
        m_next_sibling.push_back(undef_id);
        m_literal.push_back(c);
        return node_t(size() - 1, true);
    }

    inline size_t size() const {
        return m_first_child.size();
    }
};

}} //ns


