#ifndef LZSS_FACTORS_H
#define LZSS_FACTORS_H

#include <iterator>
#include <initializer_list>

#include <glog/logging.h>

#include <tudocomp/util/sdsl_extension.h>
#include <tudocomp/lzss/factor.h>

namespace lz77rule {

using namespace sdsl_extension;

class RulesIterator;
class RulesReference;

/// A specialized container class of Rules
class Rules {
public:
    typedef RulesIterator iterator;
    typedef RulesIterator const_iterator;

    typedef RulesReference reference;
    typedef RulesReference const_reference;

    typedef sdsl::int_vector<0>::size_type size_type;
    typedef sdsl::int_vector<0>::difference_type difference_type;

private:
    GrowableIntVector targets;
    GrowableIntVector sources;
    GrowableIntVector nums;
    friend RulesReference;

public:
    inline Rules() {}

    inline Rules(std::initializer_list<Rule> rules) {
        for (Rule rule : rules) {
            push_back(rule);
        }
    }

    inline const size_type size() const {
        return targets.size();
    }

    inline void push_back(Rule value) {
        targets.push_back(value.target);
        sources.push_back(value.source);
        nums.push_back(value.num);
    }
    inline const iterator begin();
    inline const iterator end();
    inline reference operator[](const size_type& i);
    inline const_reference operator[](const size_type& i) const;
};

/// A reference to an element in a Rules container
class RulesReference {
public:
    typedef Rule value_type;
private:
    sdsl::int_vector<0>::size_type m_i;
    Rules* m_rules;
public:
    inline RulesReference(sdsl::int_vector<0>::size_type i,
                                Rules* rules):
        m_i(i), m_rules(rules) {
    };

    inline RulesReference& operator=(value_type x) {
        m_rules->targets[m_i] = x.target;
        m_rules->sources[m_i] = x.source;
        m_rules->nums[m_i] = x.num;
        return *this;
    };

    inline RulesReference& operator=(const RulesReference& x) {
        return *this = value_type(x);
    };

    inline operator value_type() const {
        Rule r {
            m_rules->targets[m_i],
            m_rules->sources[m_i],
            m_rules->nums[m_i],
        };
        return r;
    }

    inline bool operator==(const RulesReference& x)const {
        return value_type(*this) == value_type(x);
    }

    inline bool operator!=(const RulesReference& x)const {
        return value_type(*this) != value_type(x);
    }
};

inline std::ostream& operator<<(std::ostream& os, const RulesReference rule) {
    return os << Rule(rule);
}

/// A Rules iterator
class RulesIterator: public std::iterator<
    std::input_iterator_tag,
    Rule,
    sdsl::int_vector<0>::difference_type,
    RulesReference*,
    RulesReference>
{
    sdsl::int_vector<0>::size_type m_i;
    Rules* m_rules;
public:
    // iterator
    inline RulesIterator(sdsl::int_vector<0>::size_type i, Rules* rules): m_i(i), m_rules(rules) {};
    inline RulesIterator(const RulesIterator& it): m_i(it.m_i), m_rules(it.m_rules) {};
    inline RulesIterator& operator++() {
        ++m_i;
        return *this;
    }

    inline RulesIterator operator++(int) {
        RulesIterator tmp(*this);
        operator++();
        return tmp;
    }

    // input/output iterator
    inline bool operator==(const RulesIterator& rhs) {
        return m_i == rhs.m_i;
    }
    inline bool operator!=(const RulesIterator& rhs) {
        return m_i != rhs.m_i;
    }

    inline RulesReference operator*() {
        return RulesReference(m_i, m_rules);
    }

    // forward iterator
    inline RulesIterator(): m_i(0), m_rules(nullptr) { }

    // bidirectional iterator
    inline RulesIterator& operator--() {
        --m_i;
        return *this;
    }

    inline RulesIterator operator--(int) {
        RulesIterator tmp(*this);
        operator--();
        return tmp;
    }

    // random access iterator
    inline friend RulesIterator operator+(RulesIterator lhs,
                                          const difference_type rhs) {
        lhs += rhs;
        return lhs;
    }
    inline friend RulesIterator operator+(const difference_type lhs,
                                          RulesIterator rhs) {
        rhs += lhs;
        return rhs;
    }
    inline friend RulesIterator operator-(RulesIterator lhs,
                                          const difference_type rhs) {
        lhs -= rhs;
        return lhs;
    }
    inline difference_type operator-(const RulesIterator& rhs) const {
        difference_type tmp = m_i - rhs.m_i;
        return tmp;
    }

    inline bool operator<(const RulesIterator& rhs) const {
        return m_i < rhs.m_i;
    }

    inline bool operator>(const RulesIterator& rhs) const {
        return rhs < *this;
    }

    inline bool operator<=(const RulesIterator& rhs) const {
        return !(*this > rhs);
    }

    inline bool operator>=(const RulesIterator& rhs) const {
        return !(*this < rhs);
    }

    inline RulesIterator& operator+=(const difference_type rhs) {
        m_i += rhs;
        return *this;
    }

    inline RulesIterator& operator-=(const difference_type rhs) {
        m_i -= rhs;
        return *this;
    }

    inline RulesReference operator[](const difference_type i) {
        RulesIterator tmp(*this);
        tmp += i;
        return *tmp;
    }
};

inline void swap(RulesReference x,
                 RulesReference y)
{
    Rule tmp = x;
    x = y;
    y = tmp;
}

inline void swap(Rule& x,
                 RulesReference y)
{
    Rule tmp = x;
    x = y;
    y = tmp;
}

inline void swap(RulesReference x,
                 Rule& y)
{
    Rule tmp = x;
    x = y;
    y = tmp;
}

inline const Rules::iterator Rules::begin() {
    return RulesIterator(0, this);
}

inline const Rules::iterator Rules::end() {
    return RulesIterator(size(), this);
}

inline Rules::reference Rules::operator[](const Rules::size_type& i) {
    return RulesReference(i, this);
}

inline Rules::const_reference Rules::operator[](const Rules::size_type& i) const {
    return RulesReference(i, const_cast<Rules*>(this));
}

}

#endif
