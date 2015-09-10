#ifndef RULES_H
#define RULE_H

#include "sdsl/int_vector.hpp"
#include "sdsl/bits.hpp"
#include "sdsl/util.hpp"
#include "glog/logging.h"
#include "rule.h"

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
    inline GrowableIntVector(): vec(0,0,1), m_size(0) {
    }

    inline void push_back(uint64_t value) {
        uint64_t value_width = sdsl::bits::hi(value) + 1;

        DLOG(INFO) << "value = " << value;
        DLOG(INFO) << "value_width = " << value_width;
        DLOG(INFO) << "vec.width() = " << int(vec.width());

        // grow width if neccessary
        if (value_width > vec.width()) {
            sdsl::util::expand_width(vec, value_width);

            // TODO: Bug in SDSL, width will not be set if empty
            vec.width(value_width);

            DLOG(INFO) << "new vec.width() = " << int(vec.width());
        }

        // grow capacity if neccessary (vec.size() is the capacity)s
        if (m_size == vec.size()) {
            // Needs cast to signed m_size in the middle so that it properly
            // calculates -1 for the m_size == 0 case.
            size_type cap = m_size + (difference_type(m_size) - 1) / 2 + 1;
            vec.resize(cap);
        }

        vec[m_size] = std::move(value);
        m_size++;
    }

    inline const iterator begin() {
        return vec.begin();
    }

    inline const iterator end() {
        return vec.end();
    }

    inline const const_iterator begin() const {
        return vec.begin();
    }

    inline const const_iterator end() const {
        return vec.end();
    }

    inline size_type size() const {
        return m_size;
    }

    inline auto operator[](const size_type& idx) -> reference {
        return vec[idx];
    }

    inline auto operator[](const size_type& idx) const -> const_reference
    {
        return vec[idx];
    }
};

class CompressedRules {
public:
    typedef sdsl::int_vector<0>::iterator iterator;
    typedef sdsl::int_vector<0>::const_iterator const_iterator;

    typedef sdsl::int_vector<0>::reference reference;
    typedef sdsl::int_vector<0>::const_reference const_reference;

    typedef sdsl::int_vector<0>::size_type size_type;
    typedef sdsl::int_vector<0>::difference_type difference_type;

private:
    GrowableIntVector targets;
    GrowableIntVector sources;
    GrowableIntVector nums;

public:
    inline CompressedRules() {
    }

    inline void push_back(Rule value) {
        DCHECK(value.target >= targets[targets.size() - 1]);
        targets.push_back(value.target);
        sources.push_back(value.source);
        nums.push_back(value.num);
    }
};

#endif
