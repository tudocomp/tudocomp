#ifndef LZ78RULES_H
#define LZ78RULES_H

#include <iterator>
#include <initializer_list>

#include <glog/logging.h>

#include <tudocomp/sdslex/GrowableIntVector.hpp>
#include <tudocomp/lz78/factor.h>

namespace lz78 {

class EntriesIterator;
class EntriesReference;

/// A specialized container class of Entries
class Entries {
public:
    typedef EntriesIterator iterator;
    typedef EntriesIterator const_iterator;

    typedef EntriesReference reference;
    typedef EntriesReference const_reference;

    typedef sdsl::int_vector<0>::size_type size_type;
    typedef sdsl::int_vector<0>::difference_type difference_type;

private:
    sdslex::GrowableIntVector indices;
    sdslex::GrowableIntVector chrs;
    friend EntriesReference;

public:
    inline Entries() {}

    inline Entries(std::initializer_list<Entry> rules) {
        for (Entry rule : rules) {
            push_back(rule);
        }
    }

    inline const size_type size() const {
        return indices.size();
    }

    inline void push_back(Entry value) {
        indices.push_back(value.index);
        chrs.push_back(value.chr);
    }
    inline const iterator begin();
    inline const iterator end();
    inline reference operator[](const size_type& i);
    inline const_reference operator[](const size_type& i) const;
};

/// A reference to an element in a Entries container
class EntriesReference {
public:
    typedef Entry value_type;
private:
    sdsl::int_vector<0>::size_type m_i;
    Entries* m_rules;
public:
    inline EntriesReference(sdsl::int_vector<0>::size_type i,
                                Entries* rules):
        m_i(i), m_rules(rules) {
    };

    inline EntriesReference& operator=(value_type x) {
        m_rules->indices[m_i] = x.index;
        m_rules->chrs[m_i] = x.chr;
        return *this;
    };

    inline EntriesReference& operator=(const EntriesReference& x) {
        return *this = value_type(x);
    };

    inline operator value_type() const {
        Entry r {
            m_rules->indices[m_i],
            uint8_t(m_rules->chrs[m_i]),
        };
        return r;
    }

    inline bool operator==(const EntriesReference& x)const {
        return value_type(*this) == value_type(x);
    }

    inline bool operator!=(const EntriesReference& x)const {
        return value_type(*this) != value_type(x);
    }
};

inline std::ostream& operator<<(std::ostream& os, const EntriesReference rule) {
    return os << Entry(rule);
}

/// A Entries iterator
class EntriesIterator: public std::iterator<
    std::input_iterator_tag,
    Entry,
    sdsl::int_vector<0>::difference_type,
    EntriesReference*,
    EntriesReference>
{
    sdsl::int_vector<0>::size_type m_i;
    Entries* m_rules;
public:
    // iterator
    inline EntriesIterator(sdsl::int_vector<0>::size_type i, Entries* rules): m_i(i), m_rules(rules) {};
    inline EntriesIterator(const EntriesIterator& it): m_i(it.m_i), m_rules(it.m_rules) {};
    inline EntriesIterator& operator++() {
        ++m_i;
        return *this;
    }

    inline EntriesIterator operator++(int) {
        EntriesIterator tmp(*this);
        operator++();
        return tmp;
    }

    // input/output iterator
    inline bool operator==(const EntriesIterator& rhs) {
        return m_i == rhs.m_i;
    }
    inline bool operator!=(const EntriesIterator& rhs) {
        return m_i != rhs.m_i;
    }

    inline EntriesReference operator*() {
        return EntriesReference(m_i, m_rules);
    }

    // forward iterator
    inline EntriesIterator(): m_i(0), m_rules(nullptr) { }

    // bidirectional iterator
    inline EntriesIterator& operator--() {
        --m_i;
        return *this;
    }

    inline EntriesIterator operator--(int) {
        EntriesIterator tmp(*this);
        operator--();
        return tmp;
    }

    // random access iterator
    inline friend EntriesIterator operator+(EntriesIterator lhs,
                                          const difference_type rhs) {
        lhs += rhs;
        return lhs;
    }
    inline friend EntriesIterator operator+(const difference_type lhs,
                                          EntriesIterator rhs) {
        rhs += lhs;
        return rhs;
    }
    inline friend EntriesIterator operator-(EntriesIterator lhs,
                                          const difference_type rhs) {
        lhs -= rhs;
        return lhs;
    }
    inline difference_type operator-(const EntriesIterator& rhs) const {
        difference_type tmp = m_i - rhs.m_i;
        return tmp;
    }

    inline bool operator<(const EntriesIterator& rhs) const {
        return m_i < rhs.m_i;
    }

    inline bool operator>(const EntriesIterator& rhs) const {
        return rhs < *this;
    }

    inline bool operator<=(const EntriesIterator& rhs) const {
        return !(*this > rhs);
    }

    inline bool operator>=(const EntriesIterator& rhs) const {
        return !(*this < rhs);
    }

    inline EntriesIterator& operator+=(const difference_type rhs) {
        m_i += rhs;
        return *this;
    }

    inline EntriesIterator& operator-=(const difference_type rhs) {
        m_i -= rhs;
        return *this;
    }

    inline EntriesReference operator[](const difference_type i) {
        EntriesIterator tmp(*this);
        tmp += i;
        return *tmp;
    }
};

inline void swap(EntriesReference x,
                 EntriesReference y)
{
    Entry tmp = x;
    x = y;
    y = tmp;
}

inline void swap(Entry& x,
                 EntriesReference y)
{
    Entry tmp = x;
    x = y;
    y = tmp;
}

inline void swap(EntriesReference x,
                 Entry& y)
{
    Entry tmp = x;
    x = y;
    y = tmp;
}

inline const Entries::iterator Entries::begin() {
    return EntriesIterator(0, this);
}

inline const Entries::iterator Entries::end() {
    return EntriesIterator(size(), this);
}

inline Entries::reference Entries::operator[](const Entries::size_type& i) {
    return EntriesReference(i, this);
}

inline Entries::const_reference Entries::operator[](const Entries::size_type& i) const {
    return EntriesReference(i, const_cast<Entries*>(this));
}

}

#endif
